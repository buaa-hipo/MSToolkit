import re
from collections import defaultdict
import json
import sys
import argparse

# 从文件读取事件定义
def read_event_definition(file_path):
    with open(file_path, 'r') as file:
        return file.read()

def parse_event_to_type_mapping_from_file(file_path, EVENT_LIST, CLASS_EVENTS):
    # 初始化 TYPE_MAP 和 TYPE_LIST
    # type_map = defaultdict(lambda: "record_t")  # 映射事件名称到记录类型
    type_map = {}  # 映射事件名称到记录类型
    type_list = defaultdict(dict)  # 记录类型详情

    current_event = None  # 当前事件
    current_type = None  # 当前类型
    base_type = None  # 当前类型的基类
    is_in_struct = False  # 标记是否在结构体定义中
    struct_name = None  # 结构体名称
    struct_desc = None  # 结构体描述
    struct_fields = []  # 结构体字段列表

    # 正则表达式匹配
    type_pattern = re.compile(r"FOR\s+(?P<event>([A-Za-z0-9_]+(\s*,\s*[A-Za-z0-9_]+)*))\s*TYPE\s+(?P<type_name>[\w_]+)\s*:\s*(?P<base_type>[\w_]+)\s+(?P<content>.*?)\s*END TYPE", re.DOTALL)
    # type_pattern = re.compile(r"TYPE\s+(?P<type_name>[\w_]+)\s*:\s*(?P<base_type>[\w_]+)")
    attribute_pattern = re.compile(r"(?P<attr_is_const>(\bconst \b)?)\s?(?P<attr_type>[\w_]+)\s?(?P<attr_is_ptr>[\*]?)\s+(?P<attr_name>[\w_]+)\s*:\s*\"(?P<attr_desc>.*?)\"")
    attribute_no_desc_pattern = re.compile(r"(?P<attr_is_const>(\bconst \b)?)\s?(?P<attr_type>[\w_]+)\s?(?P<attr_is_ptr>[\*]?)\s+(?P<attr_name>[\w_]+)\s*:\s*NONE")
    struct_pattern = re.compile(r"STRUCT\s+(?P<struct_name>[\w_]+)\s*:\s*\"(?P<struct_desc>.*?)\"")
    struct_end_pattern = re.compile(r"END STRUCT")

    # 从文件读取内容
    with open(file_path, "r") as f:
        event_definition = f.read()
        
    last_end = 0
    for type_match in type_pattern.finditer(event_definition):
        # check for non-matched part
        start, end = type_match.span()
        unknown = event_definition[last_end:start].strip()
        if len(unknown)>0:
            print("WARNNING: Unknown parts >>>>>>>")
            print(unknown)
            print("WARNNING: <<<<<<<<<<<<<<<<<<<<<")
        last_end = end
        # matched parts
        current_events = type_match.group("event")
        current_type = type_match.group("type_name")
        base_type = type_match.group("base_type")
        type_list[current_type] = {
            "base_type": base_type,
            "attribute": []
        }
        for ce_raw in current_events.split(','):
            ce = ce_raw.strip()
            if len(ce)>0:
                if ce not in EVENT_LIST:
                    found_event = False
                    for version, class_map in CLASS_EVENTS.items():
                        if ce in class_map.keys():
                            for i in range(class_map[ce]['start_event_id'], class_map[ce]['end_event_id']):
                                type_map[EVENT_LIST[i]] = current_type
                            found_event = True
                    if not found_event:
                        print("ERROR: Unknown event ", ce)
                        exit(1)
                else:
                    type_map[ce] = current_type
        content = type_match.group("content").strip()
        curr_type_def = [ type_list[current_type] ]
        for line in content.splitlines():
            # 处理结构体定义
            # print(">>> Parsing line >>> ", line)
            line = line.strip()
            # print(">>> LINE NORMALIZED >>> ", line)
            struct_match = struct_pattern.match(line)
            if struct_match:
                # print("<<< STRUCT MATCH")
                struct_name = struct_match.group("struct_name")
                struct_desc = struct_match.group("struct_desc")
                # 添加结构体到记录类型的属性列表
                struct_def = {
                    "name": struct_name,
                    "type": "STRUCT",
                    "desc": struct_desc,
                    "attribute": []
                }
                curr_type_def[-1]["attribute"].append(struct_def)
                curr_type_def.append(struct_def)
                continue
            
            struct_end_match = struct_end_pattern.match(line)
            if struct_end_match:
                # print("<<< END STRUCT MATCH")
                curr_type_def.pop()
                continue
                
            # 处理普通属性定义
            attribute_match = attribute_pattern.match(line)
            if attribute_match:
                # print("<<< ATTRIBUTE MATCH")
                attr_type = attribute_match.group("attr_type")
                attr_name = attribute_match.group("attr_name")
                attr_desc = attribute_match.group("attr_desc")
                attr_is_const = attribute_match.group("attr_is_const")
                attr_is_ptr = attribute_match.group("attr_is_ptr")

                # 添加属性到记录类型的属性列表
                curr_type_def[-1]["attribute"].append({
                    "name": attr_name,
                    "type": attr_type,
                    "desc": attr_desc,
                    "is_const" : len(attr_is_const)>0,
                    "is_ptr" : len(attr_is_ptr)>0
                })
                continue
            
            # 处理普通属性定义
            attribute_match = attribute_no_desc_pattern.match(line)
            if attribute_match:
                # print("<<< ATTRIBUTE WITHOUT DESC MATCH")
                attr_type = attribute_match.group("attr_type")
                attr_name = attribute_match.group("attr_name")
                attr_is_const = attribute_match.group("attr_is_const")
                attr_is_ptr = attribute_match.group("attr_is_ptr")

                # 添加属性到记录类型的属性列表
                curr_type_def[-1]["attribute"].append({
                    "name": attr_name,
                    "type": attr_type,
                    "desc": None,
                    "is_const" : len(attr_is_const)>0,
                    "is_ptr" : len(attr_is_ptr)>0
                })
                continue
            
            print("WARNING: UNMATCHED ATTRIBUTE LINE >>>> ", line)

    return type_map, type_list

def parse_event_classes(event_definition):
    event_list = []  # 全局事件列表
    class_events = {}  # 每个 CLASS 的事件及编号信息
    event_id = 0  # 事件编号从0开始

    # 正则匹配 CLASS 块
    events_block_pattern = re.compile(r"EVENTS\s+(?P<version>.*?):\s+(?P<block>.*?)\s*END EVENTS", re.DOTALL)
    class_pattern = re.compile(r"CLASS\s+(?P<class_name>\w+)\s*(?P<content>.*?)\s*END CLASS\s+\1", re.DOTALL)
    event_pattern = re.compile(r"([A-Za-z_]+[A-Za-z0-9_]*)")  # 匹配事件列表

    def process_class_block(version, block, parent_class=None):
        print("EVENT LIST VERSION: ", version)
        if version not in class_events.keys():
            class_events[version] = {}
        nonlocal event_id
        local_event_list = []  # 当前 CLASS 的事件列表
        block_norm = block.strip()
        print("NORMALIZED BLOCK >>>>>>")
        print(block_norm)
        print("<<<<<<<<<<<<<<<<<<<<<<<")
        # 遍历 CLASS 匹配的内容
        for class_match in class_pattern.finditer(block_norm):
            class_name = class_match.group("class_name").strip()
            content = class_match.group("content").strip()

            # 构造完整的类名
            full_class_name = f"{parent_class}.{class_name}" if parent_class else class_name

            # 获取当前类的事件编号范围
            start_event_id = event_id
            # 递归解析子类事件
            child_events = process_class_block(version, content, full_class_name)
            if len(child_events)==0:
                # 提取当前类的事件
                direct_events = []
                for event in content.strip().split(","):
                    event_match = event_pattern.match(event.strip())
                    if event_match:
                        start, end = event_match.span()
                        if start!=0 or end!=len(event):
                            print("ERROR: Unknown EVENT format:", event)
                            exit(1)
                        direct_events.append(event)
                    else:
                        print("ERROR: Unknown EVENT format:", event)
                        exit(1)
                all_events = direct_events
                # 更新全局事件列表
                event_list.extend(all_events)
                event_id += len(all_events)
            else:
                all_events = child_events
            end_event_id = event_id
            # 记录当前 CLASS 的事件及编号信息
            class_events[version][full_class_name] = {
                "events": all_events,
                "start_event": all_events[0] if all_events else None,
                "end_event": all_events[-1] if all_events else None,
                "start_event_id": start_event_id,
                "end_event_id": end_event_id,
            }

            # 将当前类的事件添加到本层事件列表
            local_event_list.extend(all_events)

        return local_event_list

    # 开始解析
    for events_match in events_block_pattern.finditer(event_definition):
        version = events_match.group("version")
        elist = process_class_block(version, events_match.group("block"))
    return event_list, class_events

# wrap_defines.h
def generate_wrap_define(EVENT_LIST, WORKDIR):
    output_include = WORKDIR+"/include/record/wrap_defines.h"
    include_header = '''
/* Generated by generate_wrap_defines.py */
#ifndef __PY_WRAP_MPI_WRAPPER_H__
#define __PY_WRAP_MPI_WRAPPER_H__
'''
    include_end = "#endif"

    if output_include:
        try:
            output = open(output_include, "w")
            output.write(include_header)
            for i,name in enumerate(EVENT_LIST):
                if name in ["JSI_PROCESS_START", "JSI_PROCESS_EXIT"]:
                    output.write("#define {fn} {fn_id}\n".format(fn=name, fn_id=i))
                else:
                    output.write("#define {fn} {fn_id}\n".format(fn='event_'+name, fn_id=i))
            output.write("#define __RECORD_MAP_IMPL ")
            for i,name in enumerate(EVENT_LIST):
                output.write("\\\n  case {fn_id}: return std::string(\"{fn}\");".format(fn=name, fn_id=i))
            output.write("\n")
            output.write("enum class event\n"
                        "{\n")
            for i,name in enumerate(EVENT_LIST):
                output.write("{fn} = {fn_id},\n".format(fn=name, fn_id=i))
            output.write('};\n')
            output.write("\n")
            output.write(include_end)
            output.close()
        except IOError:
            sys.stderr.write("Error: couldn't open file " + output_include + " for writing.\n")
            sys.exit(1)

# record_utils.h
def generate_record_utils(EVENT_LIST, TYPE_MAP, TYPE_LIST, WORKDIR):
    def generate_to_string_cases():
        cases=""
        maps = {}
        for name, rtype in TYPE_MAP.items():
            if rtype not in maps.keys():
                maps[rtype] = [name]
            else:
                maps[rtype].append(name)
        for rtype, namelist in maps.items():
            for name in namelist:
                cases += "\t\tcase event_{}:\n".format(name)
            attributes = TYPE_LIST[rtype]["attribute"]
            cases += "\t\t{" + "\n\t\t\t{}* rec = ({}*)r;\n".format(rtype,rtype)
            for attr in attributes:
                if attr["desc"] is not None:
                    if attr["type"]!="STRUCT":
                        cases += "\t\t\tss << \"{}:\" << rec->{} << \"\\n\";\n".format(attr["desc"], attr["name"])
            cases += "\t\t\tbreak;\n\t\t}\n"
        return cases
    
    def generate_record_size_cases():
        cases=""
        for name, rtype in TYPE_MAP.items():
            cases += "\t\tcase event_{}: return sizeof({});\n".format(name, rtype)
        return cases
            
    record_utils_file = WORKDIR + '/include/record/record_utils.h'
    try:
        with open(record_utils_file, "w") as output:
            output.write('''/* AUTO GENERATED BY compile_type_defines.py */
#ifndef __RECORD_UTILS_H__
#define __RECORD_UTILS_H__
#include "record/record_type.h"
#include "record/wrap_defines.h"
#include <sstream>

namespace record_utils {

inline __attribute__((always_inline))
std::string get_record_name(record_t *r) {
    if (r->MsgType<0) {
        return std::string("Extended Events ("+std::to_string(r->MsgType)+")");
    }
    switch (r->MsgType) {
        __RECORD_MAP_IMPL;
    }
    return std::string("Unknown");
}

inline __attribute__((always_inline))
size_t get_record_size(record_t* r) {
    switch(r->MsgType) {
'''
            +generate_record_size_cases()+
'''
    }
    return sizeof(record_t);
}

inline __attribute__((always_inline))
std::string to_string(record_t* r) {
    std::stringstream ss;
    ss << "Event Name: " << record_utils::get_record_name(r) << "\\n";
    ss << "Start Time: " << r->timestamps.enter << "\\n";
    ss << "End Time: "   << r->timestamps.exit  << "\\n";
    ss << "Duration: "   << r->timestamps.exit - r->timestamps.enter << "\\n";
    ss << "Record Size: "<< get_record_size(r) << "\\n";
    switch(r->MsgType) {
'''
            +generate_to_string_cases()+
'''
    }
    return ss.str();
}

};

#endif
''')
    except IOError:
        sys.stderr.write("Error: couldn't open file " + record_utils_file + " for writing.\n")
        sys.exit(1)

# record_type_info.h
def generate_type_info(EVENT_LIST, WORKDIR):
    record_type_info_file = WORKDIR+'/include/record/record_type_info.h'
    try:
        with open(record_type_info_file, "w") as output:
            output.write('#pragma once\n')
            output.write('#include "record/wrap_defines.h"\n')
            output.write('#include "record/record_type_traits.h"\n')
            output.write('#include "utils/compile_time.h"\n')
            output.write('#include "ral/time_detector.h"\n')
            output.write('#include <string_view>\n')
            output.write("inline pse::utils::EncodedStruct record_info[] = {\n")
            for i,name in enumerate(EVENT_LIST):
                output.write("pse::utils::StructEncoder<MacroToType<event_{fn}>>::get_encoded_struct(),\n".format(fn=name))
            output.write("};\n\n")

            output.write("inline long record_time_offset[] = {\n")
            for i,name in enumerate(EVENT_LIST):
                output.write(f"[](){{using T = MacroToType<event_{name}>; T t; return (char*)&pse::ral::TimeAccessor<T>::get_field(t) - (char*)&t;}}(),\n")
            output.write("};\n\n")


    except IOError:
        sys.stderr.write("Error: couldn't open file " + record_type_info_file + " for writing.\n")
        sys.exit(1)

# record_type_traits.h
def generate_type_traits(EVENT_LIST, TYPE_MAP, WORKDIR):
    record_type_traits_file = WORKDIR+'/include/record/record_type_traits.h'
    try:
        with open(record_type_traits_file, "w") as output:
            prologue = '''
#pragma once
#include "record/record_reader.h"

template <int MsgType>
struct MacroToTypeHelper
{
    using type = decltype([] {
'''
            epilogue = '''
        else
        {
            return record_t{};
        }
    }());
};

template <int MsgType>
using MacroToType = typename MacroToTypeHelper<MsgType>::type;
'''
            is_first = True
            output.write(prologue)
            for i,name in enumerate(EVENT_LIST):
                if name in TYPE_MAP.keys():
                    if is_first:
                        output.write("\n\tif constexpr (MsgType == event_"+name+")\n\t\t{\n\t\t\treturn "+TYPE_MAP[name]+"{};\n\t\t}\n")
                    else:
                        output.write("\n\telse if constexpr (MsgType == event_"+name+")\n\t\t{\n\t\t\treturn "+TYPE_MAP[name]+"{};\n\t\t}\n")
                    is_first = False
            output.write(epilogue)
    except IOError:
        sys.stderr.write("Error: couldn't open file " + record_type_traits_file + " for writing.\n")
        sys.exit(1)
        
parser = argparse.ArgumentParser(description='generate wrapper headers')
parser.add_argument('--workdir', help='workdir', default='./')
args = parser.parse_args()

WORKDIR=args.workdir

# 示例文件路径
file_path = args.workdir+'/include/record/event_defines.evt'

# 读取文件中的事件定义
event_definition = read_event_definition(file_path)

# 解析事件定义
event_list, class_events = parse_event_classes(event_definition)

# 输出事件列表
print("Event List:")
print(event_list)

# 输出每个 CLASS 的事件及编号
print("\nClass Event Details:")
for version, class_map in class_events.items():
    print(f"EVENT VERSION: ", version)
    for class_name, details in class_map.items():
        print(f"\tClass: {class_name}")
        print(f"\t  Start Event: {details['start_event']} (ID: {details['start_event_id']})")
        print(f"\t  End Event: {details['end_event']} (ID: {details['end_event_id']})")

# 示例文件路径（请确保该文件存在并包含定义内容）
file_path = args.workdir+'/include/record/record_types.def'

# 解析事件到类型的映射和类型详情
type_map, type_list = parse_event_to_type_mapping_from_file(file_path, event_list, class_events)

# 输出结果
print("TYPE_MAP:")
print(json.dumps(type_map, indent=4))

print("\nTYPE_LIST:")
print(json.dumps(type_list, indent=4))     


generate_wrap_define(event_list,WORKDIR)
generate_type_info(event_list,WORKDIR)
generate_type_traits(event_list, type_map, WORKDIR)
generate_record_utils(event_list, type_map, type_list, WORKDIR)