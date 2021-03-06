/* -*- Mode: C++; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * libgit2pp
 * Copyright (C) 2013 Émilien Kia <emilien.kia@gmail.com>
 * 
 * libgit2pp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * libgit2pp is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.";
 */

#include "repository.hpp"

#include "blob.hpp"
#include "commit.hpp"
#include "config.hpp"
#include "database.hpp"
#include "exception.hpp"
#include "index.hpp"
#include "oid.hpp"
#include "ref.hpp"
#include "signature.hpp"
#include "status.hpp"
#include "tag.hpp"
#include "tree.hpp"


#ifdef GIT_WIN32
#define GIT2PP_PATH_DIRECTORY_SEPARATOR '\\'
#else
#define GIT2PP_PATH_DIRECTORY_SEPARATOR '/'
#endif


namespace git2
{

namespace
{

struct GitRepositoryDeleter{
	void operator()(git_repository *repo){
		git_repository_free(repo);
	}
};

struct GitRepositoryNoDeleter{
	void operator()(git_repository *repo){
		/* Do nothing */
	}
};

}

//
// Repository
//

Repository::Repository(git_repository *repository, bool own)
{
	if(own)
		_repo = ptr_type(repository,  GitRepositoryDeleter());
	else
		_repo = ptr_type(repository, GitRepositoryNoDeleter());
}

Repository::Repository( const Repository& repo ):
_repo(repo._repo)
{
}

Repository::~Repository()
{
}

std::string Repository::discover(const std::string& startPath, bool acrossFs, const std::list<std::string>& ceilingDirs)
{
	std::vector<char> repoPath((size_t)GIT_PATH_MAX, (char)0);
	std::string joinedCeilingDirs;
	if(!ceilingDirs.empty())
	{
		std::list<std::string>::const_iterator iter = ceilingDirs.begin();
		joinedCeilingDirs = *iter;
		while(++iter != ceilingDirs.end())
		{
			joinedCeilingDirs += GIT_PATH_LIST_SEPARATOR;
			joinedCeilingDirs += *iter;
		}
	}
	Exception::assert(git_repository_discover(const_cast<char*>(repoPath.data()),
									  repoPath.size(), startPath.c_str(),
                                      acrossFs, joinedCeilingDirs.c_str()));
    return std::string(repoPath.data());
}

void Repository::init(const std::string& path, bool isBare)
{
	_repo.reset();
    git_repository *repo = NULL;
    Exception::assert(git_repository_init(&repo, path.c_str(), isBare));
    _repo = ptr_type(repo, GitRepositoryDeleter());
}

void Repository::open(const std::string& path)
{
	_repo.reset();
    git_repository *repo = NULL;
    Exception::assert(git_repository_open(&repo, path.c_str()));
    _repo = ptr_type(repo, GitRepositoryDeleter());
}

void Repository::discoverAndOpen(const std::string &startPath,
                                     bool acrossFs,
                                     const std::list<std::string> &ceilingDirs)
{
    open(discover(startPath, acrossFs, ceilingDirs));
}

Reference Repository::head() const
{
    git_reference *ref = NULL;
    Exception::assert(git_repository_head(&ref, _repo.get()));
    return Reference(ref);
}

bool Repository::isHeadDetached() const
{
    return Exception::assert(git_repository_head_detached(_repo.get())) == 1;
}

bool Repository::isHeadOrphan() const
{
    return Exception::assert(git_repository_head_orphan(_repo.get())) == 1;
}

bool Repository::isEmpty() const
{
    return Exception::assert(git_repository_is_empty(_repo.get())) == 1;
}

bool Repository::isBare() const
{
    return Exception::assert(git_repository_is_bare(_repo.get())) == 1;
}

std::string Repository::name() const
{
    std::string repoPath = isBare() ? path() : workDirPath();
	size_t pos = repoPath.rfind(GIT2PP_PATH_DIRECTORY_SEPARATOR);
	if(pos==std::string::npos)
		return repoPath;
	else if(pos+1<repoPath.size())
		return repoPath.substr(pos+1);
	else
		return "";
}

std::string Repository::path() const
{
    return std::string(git_repository_path(_repo.get()));
}

std::string Repository::workDirPath() const
{
    return std::string(git_repository_workdir(_repo.get()));
}

Config Repository::configuration() const
{
    git_config *cfg;
    Exception::assert( git_repository_config(&cfg, _repo.get()) );
    return Config(cfg);
}

Reference* Repository::lookupRef(const std::string& name) const
{
    git_reference *ref = NULL;
    Exception::assert(git_reference_lookup(&ref, _repo.get(), name.c_str()));
    Reference* qr = new Reference(ref);
    return qr;
}

// TODO only available from v0.18.0
/*OId* Repository::lookupRefOId(const std::string& name) const
{
    git_oid oid;
    Exception::assert(git_reference_name_to_id(&oid, _repo.get(), name.c_str()));
    OId* qoid = new OId(&oid);
    return qoid;
}*/

// TODO only available from v0.19.0
/*Reference* Repository::lookupShorthandRef(const std::string& shorthand) const
{
    git_reference *ref = NULL;
    Exception::assert(git_reference_dwim(&ref, _repo.get(), shorthand.c_str()));
    Reference* qr = new Reference(ref);
    return qr;
}*/

Commit Repository::lookupCommit(const OId& oid) const
{
    git_commit *commit = NULL;
    Exception::assert(git_commit_lookup_prefix(&commit, _repo.get(), oid.constData(), oid.length()));
    return Commit(commit);
}

Tag Repository::lookupTag(const OId& oid) const
{
    git_tag *tag = NULL;
    Exception::assert(git_tag_lookup_prefix(&tag, _repo.get(), oid.constData(), oid.length()));
    return Tag(tag);
}

Tree Repository::lookupTree(const OId& oid) const
{
    git_tree *tree = NULL;
    Exception::assert(git_tree_lookup_prefix(&tree, _repo.get(), oid.constData(), oid.length()));
    return Tree(tree);
}

Blob Repository::lookupBlob(const OId& oid) const
{
    git_blob *blob = NULL;
    Exception::assert(git_blob_lookup_prefix(&blob, _repo.get(), oid.constData(), oid.length()));
    return Blob(blob);
}

Object Repository::lookupAny(const OId &oid) const
{
    git_object *object = NULL;
    Exception::assert(git_object_lookup_prefix(&object, _repo.get(), oid.constData(), oid.length(), GIT_OBJ_ANY));
    return Object(object);
}

// TODO only available from v0.18.0
/*Reference* Repository::createRef(const std::string& name, const OId& oid, bool overwrite)
{
    git_reference *ref = NULL;
    Exception::assert(git_reference_create(&ref, _repo.get(), name.c_str(), oid.constData(), overwrite));
    Reference* qr = new Reference(ref);
    return qr;
}*/

// TODO only available from v0.18.0
/*Reference* Repository::createSymbolicRef(const std::string& name, const std::string& target, bool overwrite)
{
    git_reference *ref = NULL;
    Exception::assert(git_reference_symbolic_create(&ref, _repo.get(), name.c_str(), target.c_str(), overwrite));
    Reference* qr = new Reference(ref);
    return qr;
}*/

OId Repository::createCommit(const std::string& ref,
                                     const Signature& author,
                                     const Signature& committer,
                                     const std::string& message,
                                     const Tree& tree,
                                     const std::list<Commit>& parents)
{
    std::vector<const git_commit*> p;
	for(const Commit& parent : parents)
		p.push_back(parent.constData());

    OId oid;
    Exception::assert(git_commit_create(oid.data(), _repo.get(), ref.c_str(), author.data(), committer.data(), NULL, message.c_str(), tree.data(), p.size(), p.data()));
    return oid;
}

OId Repository::createTag(const std::string& name,
                                  const Object& target,
                                  bool overwrite)
{
    OId oid;
    Exception::assert(git_tag_create_lightweight(oid.data(), _repo.get(), name.c_str(),
                                         target.data(), overwrite));
    return oid;
}

OId Repository::createTag(const std::string& name,
                                  const Object& target,
                                  const Signature& tagger,
                                  const std::string& message,
                                  bool overwrite)
{
    OId oid;
    Exception::assert(git_tag_create(oid.data(), _repo.get(), name.c_str(), target.data(),
                             tagger.data(), message.c_str(), overwrite));
    return oid;
}

void Repository::deleteTag(const std::string& name)
{
    Exception::assert(git_tag_delete(_repo.get(), name.c_str()));
}

OId Repository::createBlobFromFile(const std::string& path)
{
    OId oid;
    Exception::assert(git_blob_create_fromdisk(oid.data(), _repo.get(), path.c_str()));
    return oid;
}

OId Repository::createBlobFromBuffer(const std::vector<unsigned char>& buffer)
{
    OId oid;
    Exception::assert(git_blob_create_frombuffer(oid.data(), _repo.get(), buffer.data(), buffer.size()));
    return oid;
}

std::list<std::string> Repository::listTags(const std::string& pattern) const
{
    std::list<std::string> list;
    git_strarray tags;
    Exception::assert(git_tag_list_match(&tags, pattern.c_str(), _repo.get()));
    for(size_t i = 0; i < tags.count; ++i)
    {
        list.push_back(std::string(tags.strings[i]));
    }
    git_strarray_free(&tags);
    return list;
}

std::list<std::string> Repository::listReferences() const
{
    std::list<std::string> list;
    git_strarray refs;
    Exception::assert(git_reference_list(&refs, _repo.get(), GIT_REF_LISTALL));
    for(size_t i = 0; i < refs.count; ++i)
    {
        list.push_back(std::string(refs.strings[i]));
    }
    git_strarray_free(&refs);
    return list;
}

Database Repository::database() const
{
    git_odb *odb;
    Exception::assert( git_repository_odb(&odb, _repo.get()) );
    return Database(odb);
}

Index Repository::index() const
{
    git_index *idx;
    Exception::assert(git_repository_index(&idx, _repo.get()));
    return Index(idx);
}

// TODO only available from v0.19.0
/*StatusList Repository::status(const StatusOptions *options) const
{
    const git_status_options opt = options->constData();
    git_status_list *statusList;
    Exception::assert(git_status_list_new(&statusList, _repo.get(), &opt));
    return StatusList(statusList);
}*/

git_repository* Repository::data() const
{
    return _repo.get();
}

const git_repository* Repository::constData() const
{
    return _repo.get();
}




} // namespace git2

