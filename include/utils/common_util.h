//
// Created by chengyanshang on 2023/2/19.
//

#ifndef JSI_TOOLKIT_COMMON_UTIL_H
#define JSI_TOOLKIT_COMMON_UTIL_H

std::vector<std::string> stringSplit(const std::string &str, char delim) {
    std::vector<std::string> result;
    std::string tmp;
    for (int i = 0; i < str.size(); i++) {
        if (str[i] == delim) {
            if (tmp.size() != 0) {
                result.push_back(tmp);
                tmp = "";
            }
        } else {
            tmp = tmp + str[i];
        }
    }
    if (tmp != "") { result.push_back(tmp); }
    return result;
}

#endif //JSI_TOOLKIT_COMMON_UTIL_H
