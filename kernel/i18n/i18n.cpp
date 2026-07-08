#include "i18n.h"
#include <string.h>

namespace i18n {

static Language current_language = LANG_ZH_CN;

static const char* english_messages[] = {
    "Welcome to XBTOS-LC Alpha\nType 'help' for available commands.\n",
    "XBTOS-LC> ",
    "Available commands:\n",
    " help clear about sysmit dirs lang\n",
    " lst touch rdf rec append rm stat\n",
    " lang en | lang zh\n",
    "Unknown command: ",
    "File created.\n",
    "File deleted.\n",
    "File not found.\n",
    "File written.\n",
    "Usage: touch <filename>\n",
    "Usage: rdf <filename>\n",
    "Usage: rec <filename> <text>\n",
    "Language switched to English.\n",
    "已切换到中文。\n",
    "XBTOS-LC Alpha 1029\nDeveloped by XBT. Technology Studio 2026\n",
    "=== System Information ===\n",
    "Current Path: /\n"
};

static const char* chinese_messages[] = {
    "欢迎使用 XBTOS-LC Alpha\n输入 help 查看命令。\n",
    "XBTOS-LC> ",
    "可用命令：\n",
    " help clear about sysmit dirs lang\n",
    " lst touch rdf rec append rm stat\n",
    " lang en | lang zh\n",
    "未知命令：",
    "文件已创建。\n",
    "文件已删除。\n",
    "文件不存在。\n",
    "文件已写入。\n",
    "用法：touch <文件名>\n",
    "用法：rdf <文件名>\n",
    "用法：rec <文件名> <文本>\n",
    "Language switched to English.\n",
    "已切换到中文。\n",
    "XBTOS-LC Alpha 1029\n由 XBT. Technology Studio 开发，2026\n",
    "=== 系统信息 ===\n",
    "当前路径：/\n"
};

void set_language(Language language) {
    current_language = language;
}

Language get_language() {
    return current_language;
}

const char* text(MessageId id) {
    int index = (int)id;
    if (current_language == LANG_ZH_CN) {
        return chinese_messages[index];
    }
    return english_messages[index];
}

bool set_language_by_name(const char* name) {
    if (!name) {
        return false;
    }
    if (kstrcmp(name, "zh") == 0 || kstrcmp(name, "cn") == 0 || kstrcmp(name, "zh-cn") == 0) {
        set_language(LANG_ZH_CN);
        return true;
    }
    if (kstrcmp(name, "en") == 0 || kstrcmp(name, "us") == 0) {
        set_language(LANG_EN);
        return true;
    }
    return false;
}

} // namespace i18n
