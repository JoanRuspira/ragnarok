import yaml
import json

def yaml_loader(filepath):
    with open(filepath) as file_descriptor:
        data = yaml.safe_load(file_descriptor)
    return data

def load_items(filepath):
    data = yaml_loader(filepath)
    items = []
    for item in data.get("Body", ""):
        items.append(item.get("AegisName").strip())
    return items

def load_full_items():
    items = []
    items.append(load_items("../db/re/item_db_1hswords.yml"))
    items.append(load_items("../db/re/item_db_2hswords.yml"))
    items.append(load_items("../db/re/item_db_1hspears.yml"))
    items.append(load_items("../db/re/item_db_1hspears.yml"))
    items.append(load_items("../db/re/item_db_1haxes.yml"))
    items.append(load_items("../db/re/item_db_2haxes.yml"))
    items.append(load_items("../db/re/item_db_1hstaffs.yml"))
    items.append(load_items("../db/re/item_db_2hstaffs.yml"))
    items.append(load_items("../db/re/item_db_knuckles.yml"))
    items.append(load_items("../db/re/item_db_instruments.yml"))
    items.append(load_items("../db/re/item_db_katars.yml"))
    items.append(load_items("../db/re/item_db_books.yml"))
    items.append(load_items("../db/re/item_db_daggers.yml"))
    items.append(load_items("../db/re/item_db_bows.yml"))
    items.append(load_items("../db/re/item_db_maces.yml"))
    items.append(load_items("../db/re/item_db_headgears.yml"))
    items.append(load_items("../db/re/item_db_armors.yml"))
    items.append(load_items("../db/re/item_db_shields.yml"))
    items.append(load_items("../db/re/item_db_garments.yml"))
    items.append(load_items("../db/re/item_db_footgears.yml"))
    items.append(load_items("../db/re/item_db_accessories.yml"))
    items.append(load_items("../db/re/item_db_usable.yml"))
    items.append(load_items("../db/re/item_db_etc_drops.yml"))
    items.append(load_items("../db/re/item_db_etc_skill_requirements.yml"))
    items.append(load_items("../db/re/item_db_cards.yml"))
    return items

if __name__ == "__main__":
    # items = load_full_items()
    shop_items = []
    with open("../npc/merchants/shops.txt", encoding='ANSI') as f:
        lines = f.readlines()
        for line in lines:
            if not line.startswith('//'):
                post_hash = line.split("#")[-1].split(	)
                if post_hash != []:
                    shop_items.append(post_hash[-1].split(",")[-1].replace(":-1",""))


    for shop_item in shop_items:
        with open('current.lua', encoding='ANSI') as f:
            if f"[{shop_item}]" not in f.read():
                print(shop_item)
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    # data = yaml_loader("../npc/npcorchestra/templates/dynamic/mob_db_template.yml")
    # non_items = []
    # body = []
    # for monster in data.get("Body", ""):
    #     mob_drops = monster.get("Drops")
    #     if mob_drops:
    #         body_mob_drops = []
    #         for mob_drop in mob_drops:
    #             if not any(mob_drop.get("Item").strip() in x for x in items):
    #                 non_items.append(mob_drop.get("Item").strip())
    #             else:
    #                 body_mob_drops.append(mob_drop)
    #         monster["Drops"] = body_mob_drops
 
    #     body.append(monster)

    
    # with open("../npc/npcorchestra/templates/dynamic/mob_db_template2.yml", 'w') as file:
    #     documents = yaml.dump(body, file, sort_keys=False)








    # filepath = "../db/re/item_db_usable.yml"
    # data = yaml_loader(filepath)

    # item_names = []

    # body = []
    # for item in data.get("Body", ""):
    #     # if(item.get("AegisName") not in drops):
    #     #     item_names.append(item.get("AegisName"))
    #     if(item.get("AegisName") in drops):
    #         body.append(item)
        
    # # print(len(item_names))        
    # # print(item_names)       
        
    # with open(filepath, 'w') as file:
    #     documents = yaml.dump(body, file, sort_keys=False)
