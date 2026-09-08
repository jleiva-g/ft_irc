enum	Numeric {
	RPL_WELCOME				= 001,
	RPL_YOURHOST			= 002,
	RPL_CREATED				= 003,
	RPL_MYINFO				= 004,
	RPL_NOTOPIC				= 331,
	RPL_TOPIC				= 332,
	RPL_NAMREPLY			= 353,
	RPL_ENDOFNAMES			= 366,
	ERR_NOSUCHNICK			= 401,
	ERR_NOSUCHCHANNEL		= 403,
	ERR_CANNOTSENDTOCHAN	= 404,
	ERR_TOOMANYCHANNELS		= 405,
	ERR_NORECIPIENT			= 411,
	ERR_NOTEXTTOSEND		= 412,
	ERR_UNKNOWNCOMMAND		= 421,
	ERR_NOMOTD				= 422,
	ERR_NONICKNAMEGIVEN		= 431,
	ERR_ERRONEUSNICKNAME	= 432,
	ERR_NICKNAMEINUSE		= 433,
	ERR_USERNOTINCHANNEL	= 441,
	ERR_NOTONCHANNEL		= 442,
	ERR_USERONCHANNEL		= 443,
	ERR_NOTREGISTERED		= 451,
	ERR_NEEDMOREPARAMS		= 461,
	ERR_ALREADYREGISTERED	= 462,
	ERR_PASSWDMISMATCH		= 464,
	ERR_CHANNELISFULL		= 471,
	ERR_UNKNOWNMODE			= 472,
	ERR_INVITEONLYCHAN		= 473,
	ERR_BADCHANNELKEY		= 475,
	ERR_BADCHANMASK			= 476,
	ERR_CHANOPRIVSNEEDED	= 482
};

struct NumericInfo {
	Numeric		code;
	const string	message;
};

static const NumericInfo NUMERIC_MESSAGES[] = {
	{RPL_WELCOME,				":Welcome to the Internet Relay Network"},
	{RPL_YOURHOST,				":Your host is"},
	{RPL_CREATED,				":This server was created"},
	{RPL_MYINFO,				":Server info:"},
	{RPL_NOTOPIC,				":No topic is set"},
	{RPL_TOPIC,					""},
	{RPL_NAMREPLY,				""},
	{RPL_ENDOFNAMES,			":End of /NAMES list"},
	{ERR_NOSUCHNICK,			":No such nick/channel"},
	{ERR_NOSUCHCHANNEL,			":No such channel"},
	{ERR_CANNOTSENDTOCHAN,		":Cannot send to channel"},
	{ERR_TOOMANYCHANNELS,		":You have joined too many channels"},
	{ERR_NORECIPIENT,			":No recipient given"},
	{ERR_NOTEXTTOSEND,			":No text to send"},
	{ERR_UNKNOWNCOMMAND,		":Unknown command"},
	{ERR_NOMOTD,				":MOTD File is missing"},
	{ERR_NONICKNAMEGIVEN,		":No nickname given"},
	{ERR_ERRONEUSNICKNAME,		":Erroneous nickname"},
	{ERR_NICKNAMEINUSE,			":Nickname is already in use"},
	{ERR_USERNOTINCHANNEL,		":They aren't on that channel"},
	{ERR_NOTONCHANNEL,			":You're not on that channel"},
	{ERR_USERONCHANNEL,			":is already on channel"},
	{ERR_NOTREGISTERED,			":You have not registered"},
	{ERR_NEEDMOREPARAMS,		":Not enough parameters"},
	{ERR_ALREADYREGISTERED,		":You may not reregister"},
	{ERR_PASSWDMISMATCH,		":Password incorrect"},
	{ERR_CHANNELISFULL,			":Cannot join channel (+l)"},
	{ERR_UNKNOWNMODE,			":is unknown mode char to me"},
	{ERR_INVITEONLYCHAN,		":Cannot join channel (+i)"},
	{ERR_BADCHANNELKEY,			":Cannot join channel (+k)"},
	{ERR_BADCHANMASK,			":Bad Channel Mask"},
	{ERR_CHANOPRIVSNEEDED,		":You're not channel operator"}
};

NumericInfo getNumericInfo(Numeric code) {
	for (size_t i = 0; i < sizeof(NUMERIC_MESSAGES) / sizeof(NUMERIC_MESSAGES[0]); ++i) {
		if (NUMERIC_MESSAGES[i].code == code)
			return NUMERIC_MESSAGES[i];
	}
	NumericInfo unknown = {ERR_UNKNOWNCOMMAND, "Unknown command"};
	return unknown;
}
