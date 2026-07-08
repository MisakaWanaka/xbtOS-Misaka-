#ifndef XBTOS_I18N_I18N_H
#define XBTOS_I18N_I18N_H

namespace i18n {

enum Language {
    LANG_EN = 0,
    LANG_ZH_CN = 1
};

enum MessageId {
    MSG_WELCOME,
    MSG_PROMPT,
    MSG_HELP_HEADER,
    MSG_HELP_CORE,
    MSG_HELP_FILES,
    MSG_HELP_LANG,
    MSG_UNKNOWN_COMMAND,
    MSG_FILE_CREATED,
    MSG_FILE_DELETED,
    MSG_FILE_NOT_FOUND,
    MSG_FILE_WRITE_OK,
    MSG_USAGE_TOUCH,
    MSG_USAGE_READ,
    MSG_USAGE_WRITE,
    MSG_LANGUAGE_EN,
    MSG_LANGUAGE_ZH,
    MSG_ABOUT,
    MSG_SYSINFO_HEADER,
    MSG_CURRENT_PATH
};

void set_language(Language language);
Language get_language();
const char* text(MessageId id);
bool set_language_by_name(const char* name);

} // namespace i18n

#endif
