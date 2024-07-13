import yaml
import json

def yaml_loader(filepath):
    with open(filepath) as file_descriptor:
        data = yaml.safe_load(file_descriptor)
    return data

if __name__ == "__main__":
    filepath = "../npc/npcorchestra/templates/dynamic/mob_db_template.yml"
    data = yaml_loader(filepath)
    drops = []

    for monster in data.get("Body", ""):
        mob_drops = monster.get("Drops")
        if mob_drops:
            for mob_drop in mob_drops:
                drops.append(mob_drop.get("Item"))
        
    
    filepath = "../db/re/item_db_usable.yml"
    data = yaml_loader(filepath)

    item_names = []

    body = []
    for item in data.get("Body", ""):
        if(item.get("AegisName") not in drops):
            item_names.append(item.get("AegisName"))
        # if(item.get("AegisName") in drops):
        #     body.append(item)
        
    print(len(item_names))        
    # print(item_names)       
        
    # with open(filepath, 'w') as file:
    #     documents = yaml.dump(body, file, sort_keys=False)

