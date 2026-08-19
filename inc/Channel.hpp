#pragma once

#include <string>
#include <set>

using std::string;
using std::set;

class	Channel {
	private:
		string		_name;
		string		_topic;
		string		_key;
		set<int>	_members;
		set<int>	_operators;
		bool		_inviteOnly;
		bool		_topicOpOnly;
		bool		_hasKey;
		bool		_hasUserLimit;
		size_t		_userLimit;
	public:
		Channel(const string& name);
		void			addMember(int fd);
		void			removeMember(int fd);
		void			addOperator(int fd);
		void			removeOperator(int fd);
		bool			isMember(int fd) const;
		bool			isOperator(int fd) const;
		const string&	getName() const;
		const string&	getTopic() const;
		const set<int>&	getMembers() const;
		const set<int>&	getOperators() const;
		bool			isInviteOnly() const;
		bool			isTopicOpOnly() const;
		bool			hasKey() const;
		bool			hasUserLimit() const;
		const string&	getKey() const;
		size_t			getUserLimit() const;
};