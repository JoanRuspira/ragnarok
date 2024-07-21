import json
import re
import yaml
import json
import os

from encodings import euc_kr

def yaml_loader(filepath):
    with open(filepath) as file_descriptor:
        data = yaml.safe_load(file_descriptor)
    return data

def get_item_from_line(line):
    item = line.strip()
    item = item.replace("[", "")
    item = item.replace("]", "")
    item = item.replace("=", "")
    item = item.replace(" ", "")
    item = item.replace("{", "")
    return int(item)



if __name__ == "__main__":
    db_file_path = "../db/re/item_db_books.yml"
    output_file_name = "output.lua"
    full_items_file_name = "full.lua"


    try:
        os.remove(f"./{output_file_name}")
    except OSError:
        pass

    data = yaml_loader(db_file_path)
    db_items = []
    for etc_item in data.get("Body", ""):
        db_items.append(etc_item.get("Id"))

    start_item_pattern = re.compile("\[[0-9000]+\] = \{")
    keep_line = True

    newfile = []
    last_line = ""
    with open(full_items_file_name, encoding='ANSI') as f:
        lines = f.readlines()
        for line in lines:
            if keep_line:
                newfile.append(line)
            if start_item_pattern.match(line.strip()):
                item = get_item_from_line(line)
                if item in db_items:
                    keep_line = True
                    newfile_line = newfile[-1].strip()
                    newfile_item = get_item_from_line(newfile_line)
                    if start_item_pattern.match(newfile_line) and newfile_item not in db_items:
                        del newfile[-1]
                    if line.strip() != newfile[-1].strip():
                        newfile.append(line)
                else:
                    keep_line = False
            last_line = line.strip()

    with open(output_file_name, 'wb') as f:
        for line in newfile:
            f.write(line.encode('ANSI'))
