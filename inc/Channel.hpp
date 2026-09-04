#pragma once

#include <string>
#include <set>

using std::string;
using std::set;

class Client;

class	Channel {
	private:
		string			name;
		string			topic;
		string			key;
		set<Client*>	members;
		set<Client*>	operators;
		bool			inviteOnly;
		bool			topicOpOnly;
		bool			hasKey;
		long			userLimit;
	public:
		Channel();
		Channel(const string& name);
		void				addMember(Client* client);
		void				removeMember(Client* client);
		void				addOperator(Client* client);
		void				removeOperator(Client* client);
		bool				isMember(Client* client) const;
		bool				isOperator(Client* client) const;
		const string&		getName() const { return name; }
		const string&		getTopic() const { return topic; }
		const set<Client*>&	getMembers() const { return members; }
		const set<Client*>&	getOperators() const { return operators; }
		bool				isInviteOnly() const { return inviteOnly; }
		bool				isTopicOpOnly() const { return topicOpOnly; }
		bool				itHasKey() const { return hasKey; }
		bool				itHasUserLimit() const { return userLimit != -1; }
		const string&		getKey() const { return key; }
		size_t				getUserLimit() const { return userLimit; }
		void				setName(const string& value) { name = value; }
		void				setTopic(const string& value) { topic = value; }
		void				setKey(const string& value) { key = value; }
		void				setInviteOnly(bool value) { inviteOnly = value; }
		void				setTopicOpOnly(bool value) { topicOpOnly = value; }
		void				setHasKey(bool value) { hasKey = value; }
		void				setUserLimit(size_t value) { userLimit = value; }
};