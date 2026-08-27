#pragma once

#include <string>
#include <set>

using std::string;
using std::set;

class Client;

class	Channel {
	private:
		string			_name;
		string			_topic;
		string			_key;
		set<Client*>	_members;
		set<Client*>	_operators;
		bool			_inviteOnly;
		bool			_topicOpOnly;
		bool			_hasKey;
		bool			_hasUserLimit;
		size_t			_userLimit;
	public:
		Channel(const string& name);
		void				addMember(Client* client);
		void				removeMember(Client* client);
		void				addOperator(Client* client);
		void				removeOperator(Client* client);
		bool				isMember(Client* client) const;
		bool				isOperator(Client* client) const;
		const string&		getName() const;
		const string&		getTopic() const;
		const set<Client*>&	getMembers() const;
		const set<Client*>&	getOperators() const;
		bool				isInviteOnly() const;
		bool				isTopicOpOnly() const;
		bool				hasKey() const;
		bool				hasUserLimit() const;
		const string&		getKey() const;
		size_t				getUserLimit() const;
};