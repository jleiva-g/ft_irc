	#pragma once

	#include <string>
	#include <set>

	using std::string;
	using std::set;

	class Client;

	class Channel {
		private:
			enum Mode {
				MODE_INVITE_ONLY = 1 << 0,
				MODE_TOPIC_OP_ONLY = 1 << 1,
				MODE_KEY = 1 << 2,
				MODE_USER_LIMIT = 1 << 3
			};
			string			name;
			string			topic;
			string			key;
			set<Client*>	members;
			set<Client*>	operators;
			set<Client*>	invited;
			unsigned int	modes;
			long			userLimit;

		public:
			Channel();
			Channel(const string& name);

			void				addMember(Client* client);
			void				removeMember(Client* client);
			void				addOperator(Client* client);
			void				removeOperator(Client* client);
			void				addInvited(Client* client);
			void				removeInvited(Client* client);
			void				changeMode(char mode, bool enable);

			const string&		getName() const;
			const string&		getTopic() const;
			const set<Client*>&	getMembers() const;
			const set<Client*>&	getOperators() const;
			const string&		getKey() const;
			size_t				getUserLimit() const;

			bool				isMember(Client* client) const;
			bool				isOperator(Client* client) const;
			bool				isInvited(Client * Client) const;

			bool				isInviteOnly() const;
			bool				isTopicOpOnly() const;
			bool				itHasKey() const;
			bool				itHasUserLimit() const;

			void				setName(const string& value);
			void				setTopic(const string& value);
			void				setKey(const string& value);
			void				setInviteOnly(bool value);
			void				setTopicOpOnly(bool value);
			void				setHasKey(bool value);
			void				setUserLimit(size_t value);
	};
