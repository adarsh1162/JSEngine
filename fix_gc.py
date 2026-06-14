import re

with open('src/gc.cpp', 'r') as f:
    c = f.read()

c = c.replace('std::unordered_map<std::string, Value>()', 'std::unordered_map<std::string, PropertyDescriptor>()')
c = c.replace(
    'for (auto& pair : instance->fields) markValue(pair.second);',
    'for (auto& pair : instance->fields) { markValue(pair.second.value); if (pair.second.get) markObject((Obj*)pair.second.get); if (pair.second.set) markObject((Obj*)pair.second.set); }'
)

with open('src/gc.cpp', 'w') as f:
    f.write(c)

with open('src/vm.cpp', 'r') as f:
    c = f.read()

c = c.replace('push(it->second);', 'push(it->second.value);')
c = c.replace('instance->fields[name->chars] = peek(0);', 'instance->fields[name->chars].value = peek(0);')
c = c.replace('inst->fields[key] = value;', 'inst->fields[key].value = value;')

with open('src/vm.cpp', 'w') as f:
    f.write(c)
