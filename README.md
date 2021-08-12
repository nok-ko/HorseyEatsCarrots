# IGNORE THIS


gcc csprng.c -I./include -L./libs -lgcrypt -lgpg-error -L. -lcubiomes -lm -lpthread -Wl,-rpath=./libs/ -lminecraft_nether_gen_rs -o seed -Ofast

gcc testing.c -I./include -L./libs -lgcrypt -lgpg-error -L. -lcubiomes -lm -lpthread -Wl,-rpath=./libs/ -lminecraft_nether_gen_rs -o testme -Ofast

gcc lootbug.c -I./include -L./libs -lgcrypt -lgpg-error -L. -lcubiomes -lm -lpthread -Wl,-rpath=./libs/ -lminecraft_nether_gen_rs -o loot -Ofast

gcc packer.c -I./include -L./libs -lgcrypt -lgpg-error -L. -lcubiomes -lm -lpthread -Wl,-rpath=./libs/ -lminecraft_nether_gen_rs -o storeme -Ofast


int chests[13][3] ={2, 2, 0, 8, 2, 6, 3, 3, 6, 3, 3, 2, 4, 3, 2, 1, 1, 4, 0, 1, 2, 4, 4, 2, 4, 1, 0, 2, 1, 7, 4, 3, 3, 9, 1, 9, 9, 2, 3};