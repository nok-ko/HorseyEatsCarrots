#include "finders.h"
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>
#define MAXBUFLEN 60000
#define VERSION 299 //id for the validation
#define FILTER_TYPE 0// 0=> RSG overworld (Practice Seeds); 1=> village only; 2=> shipwreck only; 3 => jungle only; 4 => coinflip (classic); 5=> whatever I want; 6 => loot testing
#define DEBUG 0
#include <string.h>
#include <time.h>
#include "./minecraft_nether_gen_rs.h"

int gmir = 0;
int grot = 0;
int gtyp = 0;

int isCrying(int x, int y, int z){
  long l = (long)(x * 3129871) ^ (long)z * 116129781L ^ (long)y;
  int64_t seed = ((l * l * 42317861L + l * 11L) >> 16) ^ 0x5deece66dUL;
  return nextFloat(&seed) < .15;
}

void int64ToChar(unsigned char a[], int64_t n) {
  a[0] = (n >> 56) & 0xFF;
  a[1] = (n >> 48) & 0xFF;
  a[2] = (n >> 40) & 0xFF;
  a[3] = (n >> 32) & 0xFF;
  a[4] = (n >> 24) & 0xFF;
  a[5] = (n >> 16) & 0xFF;
  a[6] = (n >> 8) & 0xFF;
  a[7] = n & 0xFF;
}

uint64_t charTo64bitNum(char a[]) {
  uint64_t n = (unsigned long) (a[7] & 0xFF);
  n |= (unsigned long) (a[6] & 0xFF) << 8;
  n |= (unsigned long) (a[5] & 0xFF) << 16;
  n |= (unsigned long) (a[4] & 0xFF) << 24;
  n |= (unsigned long) (a[3] & 0xFF) << 32;
  n |= (unsigned long) (a[2] & 0xFF) << 40;
  n |= (unsigned long) (a[1] & 0xFF) << 48;
  n |= (unsigned long) (a[0] & 0xFF) << 56;
  return n;
}

void print64(int64_t n){
  unsigned char chars[8];
  int64ToChar(chars, n);
  int i;
  for (i = 0; i < 8; i++){
    printf("%02x", chars[i]); /* print the result */
  }
  return;
}

void print32(unsigned int n){
  unsigned char chars[4];
  chars[0] = (n >> 24) & 0xFF;
  chars[1] = (n >> 16) & 0xFF;
  chars[2] = (n >> 8) & 0xFF;
  chars[3] = n & 0xFF;
  int i;
  for (i = 0; i < 4; i++){
    printf("%02x", chars[i]); /* print the result */
  }
  return;
}

long l2norm(long x1, long z1, long x2, long z2){
  return (x1-x2)*(x1-x2) + (z1-z2)*(z1-z2);
}

void verification_token(uint64_t timestamp, unsigned int vrs, int high_int, int lower_int, int64_t seed, uint64_t IV, uint64_t timestamp2){
  printf("Verification Token:\n");
  print64(IV);
  printf("-");
  print32(high_int);
  printf("-");
  print32(lower_int);
  printf("-");
  print64(timestamp);
  printf("-");
  print32(vrs);
  printf("-");
  print64(seed);
  printf("-%ld",timestamp2-timestamp);
  printf("\n");
  return;
}

uint64_t rand64()
{
  uint64_t rv = 0;
  int c,i;
  FILE *fp;
  fp = fopen("/dev/urandom", "r");

  for (i=0; i < sizeof(rv); i++) {
     do {
       c = fgetc(fp);
     } while (c < 0);
     rv = (rv << 8) | (c & 0xff);
  }
  fclose(fp);
  return rv;
}

//THE FILTERS: ethos these output a 0 for fail 1 for succeed and check either lower48 bits (fast filter) or top16 (slow filter)

//this is classic FSG fastion without biome (pos/pos bastion, tells you neg/pos or pos/neg fortress)
char netherchecker(int64_t seed, int* fortressQuadrant){
  //return true if the nether is good (3 structures within -128 to 128 ignoring neg/neg) at 
  unsigned long modulus = 1ULL << 48;
  unsigned long AA = 341873128712;
  unsigned long BB = 132897987541;
  int bastionCount = 0;
  *fortressQuadrant = 0;
  int64_t fakeseed = (seed + 30084232ULL) ^ 0x5deece66dUL;
  int64_t chunkx = next(&fakeseed, 31) % 23;
  int64_t chunkz = next(&fakeseed, 31) % 23;
  int structureType = (next(&fakeseed, 31) % 5)  >= 2;
  bastionCount += structureType;
  if (chunkx > 7 || chunkz > 7 || structureType == 0){
    return 0;
  }
  int gotfort = 0;
  fakeseed = (seed + 30084232UL - AA) ^ 0x5deece66dUL;
  chunkx = next(&fakeseed, 31) % 23;
  chunkz = next(&fakeseed, 31) % 23;
  structureType = (next(&fakeseed, 31) % 5)  >= 2;
  bastionCount += structureType;
  if (structureType == 0){
    *fortressQuadrant = -1;
  }
  if (chunkx >= 19 && chunkz <= 8 && structureType == 0){
    return 1;
  }

  fakeseed = (seed + 30084232UL - BB) ^ 0x5deece66dUL;
  chunkx = next(&fakeseed, 31) % 23;
  chunkz = next(&fakeseed, 31) % 23;
  structureType = (next(&fakeseed, 31) % 5)  >= 2;
  bastionCount += structureType;
  if (structureType == 0){
    *fortressQuadrant = 1;
  }
  if (chunkx <= 8 && chunkz >= 19 && structureType == 0){
    return 1;
  }

  return 0;
}

//checks for basalt deltas at bastion location
char bastionbiome(uint64_t seed){
  int64_t fakeseed = (seed + 30084232ULL) ^ 0x5deece66dUL;
  int64_t chunkx = next(&fakeseed, 31) % 23;
  int64_t chunkz = next(&fakeseed, 31) % 23;
  NetherGen* netherGen=create_new_nether(seed);  
  NetherBiomes biome=get_biome(netherGen,chunkx*16,0,chunkz*16);
  if (biome==BasaltDeltas){
    delete(netherGen);
    return 0;
  }
  delete(netherGen);
  return 1;
}

//casts a drag net looking for any ravine start that will be low, wide, and flat
int ravinePositionAndQuality(int64_t seed){
  int64_t fakeseed, carvea, carveb;
  int carver_offset = 0;
  long chx, chz, i;
  long magmax, magmaz, magmay;
  long partialy, maxLength;
  float temp, pitch, width;
  for (chx = -4; chx < 16; chx++){
    for (chz = -4; chz < 16; chz++){
      fakeseed = (seed + carver_offset) ^ 0x5deece66dL;
      carvea = nextLong(&fakeseed);
      carveb = nextLong(&fakeseed);
      fakeseed = ((chx * carvea) ^ (chz * carveb) ^ (seed + carver_offset)) ^ 0x5deece66dL;
      fakeseed = fakeseed & 0xFFFFFFFFFFFF;
      temp = nextFloat(&fakeseed);
      if (temp < .02){
        Pos pos;
        magmax = chx * 16 + nextInt(&fakeseed, 16);
        partialy = nextInt(&fakeseed, 40) + 8;
        magmay = 20 + nextInt(&fakeseed, partialy);
        magmaz = chz * 16 + nextInt(&fakeseed, 16);
        pos.x = magmax;
        pos.z = magmaz;
        nextFloat(&fakeseed);
        pitch = (nextFloat(&fakeseed) - 0.5F) * 2.0F / 8.0F;
        temp = nextFloat(&fakeseed);
        width = (temp*2.0F + nextFloat(&fakeseed))*2.0F;
        maxLength = 112L - nextInt(&fakeseed, 28);
        if ( magmay < 25 && (pitch < 0.11 && pitch > -.11 ) && width > 2.5){
          return 1;
        }
      }
    }
  }
  return 0;
}

//casts a drag net for low, wide, flat, and ocean (not warm)
int ravineBiome(int64_t seed, LayerStack* gp){
  int64_t fakeseed, carvea, carveb;
  int carver_offset = 0;
  long chx, chz, i;
  long magmax, magmaz, magmay;
  long partialy, maxLength;
  float temp, pitch, width;
  for (chx = -4; chx < 16; chx++){
    for (chz = -4; chz < 16; chz++){
      fakeseed = (seed + carver_offset) ^ 0x5deece66dL;
      carvea = nextLong(&fakeseed);
      carveb = nextLong(&fakeseed);
      fakeseed = ((chx * carvea) ^ (chz * carveb) ^ (seed + carver_offset)) ^ 0x5deece66dL;
      fakeseed = fakeseed & 0xFFFFFFFFFFFF;
      temp = nextFloat(&fakeseed);
      if (temp < .02){
        Pos pos;
        magmax = chx * 16 + nextInt(&fakeseed, 16);
        partialy = nextInt(&fakeseed, 40) + 8;
        magmay = 20 + nextInt(&fakeseed, partialy);
        magmaz = chz * 16 + nextInt(&fakeseed, 16);
        pos.x = magmax;
        pos.z = magmaz;
        nextFloat(&fakeseed);
        pitch = (nextFloat(&fakeseed) - 0.5F) * 2.0F / 8.0F;
        temp = nextFloat(&fakeseed);
        width = (temp*2.0F + nextFloat(&fakeseed))*2.0F;
        maxLength = 112L - nextInt(&fakeseed, 28);
        if ( magmay < 25 && (pitch < 0.11 && pitch > -.11 ) && width > 2.5){
          int biomeAtPos = getBiomeAtPos(gp, pos);
          if (isOceanic(biomeAtPos) && (biomeAtPos !=  lukewarm_ocean &&  biomeAtPos != deep_lukewarm_ocean && biomeAtPos != warm_ocean && biomeAtPos != deep_warm_ocean) ){
            return 1;
          }
        }
      }
    }
  }
  return 0;
}

//Puts a village within 0 to 96
int villageLocation(int64_t lower48){
  const StructureConfig sconf = VILLAGE_CONFIG;
  int valid;
  Pos p = getStructurePos(sconf, lower48, 0, 0, &valid);
  if (!valid || p.x >= 112 || p.z >= 96 ){
    return 0;
  }
  return 1;
}

//Puts a jungle temple within 0 to 96
int jungleLocation(int64_t lower48){
  const StructureConfig sconf = JUNGLE_PYRAMID_CONFIG;
  int valid;
  Pos p = getStructurePos(sconf, lower48, 0, 0, &valid);
  if (!valid || p.x > 96 || p.z > 96){
    return 0;
  }
  return 1;
}

int shipwreckLocationAndType(int64_t seed){ //we will presume ocean not beach (test for speed)
  int valid3;
  const StructureConfig sconf_shipwreck = SHIPWRECK_CONFIG;
  Pos p3 = getStructurePos(sconf_shipwreck, seed, 0, 0, &valid3);
  if (!valid3 || p3.x >= 96 || p3.z >= 96){
    return 0;
  }
  unsigned long modulus = 1UL << 48;
  int64_t fakeseed, carvea, carveb;
  int shipchunkx, shipchunkz, portOceanType, portNormalY, portNormalType;
  int shiptype;
  shipchunkx = p3.x >> 4;
  shipchunkz = p3.z >> 4;
  fakeseed = (seed) ^ 0x5deece66dL;
  carvea = nextLong(&fakeseed);
  carveb = nextLong(&fakeseed);
  fakeseed = ((shipchunkx * carvea) ^ (shipchunkz * carveb) ^ seed) ^ 0x5deece66dL;
  fakeseed = fakeseed & 0xFFFFFFFFFFFF;
  fakeseed = (0x5deece66dUL*fakeseed + 11) % modulus ; //advance 2
  fakeseed = (0x5deece66dUL*fakeseed + 11) % modulus ;
  shiptype = (fakeseed >> 17) % 20;
  if (shiptype == 2 || shiptype == 5 || shiptype == 8 || shiptype == 12 || shiptype == 15 || shiptype == 18){
    return 0; //rejecting front only ships allowing all others
  }
  return 1;
}

int portalLocation(int64_t seed, int villageMode, int* px, int* pz){
  const StructureConfig sconf_portal = RUINED_PORTAL_CONFIG;
  int valid2;
  Pos p2 = getStructurePos(sconf_portal, seed, 0, 0, &valid2);
  if (villageMode == 5){
    *px = p2.x;
    *pz = p2.z;
    return 1;    
  }
  if (villageMode == 1){
    if (!valid2 || p2.x >= 64 || p2.z >= 96){// || p2.x >= 144 || p2.z >= 144){
      return 0;
    }
    *px = p2.x;
    *pz = p2.z;
    return 1;
  }
  if (!valid2 || p2.x > 144 || p2.z > 144){
    return 0;
  }
  *px = p2.x;
  *pz = p2.z;
  return 1;
}

int portalBiome(int64_t seed, LayerStack* gp){
  const StructureConfig sconf = RUINED_PORTAL_CONFIG;
  int valid;
  Pos p = getStructurePos(sconf, seed, 0, 0, &valid);
  int mc = MC_1_16;
  int biome = getBiomeAtPos(gp, p);
  if (biome % 128 == plains || biome == savanna_plateau || biome == savanna){
    return 1;
  }
  return 0;
}

//there are 3 biome classes to handle when filtering ruined_portal TYPE
int portalTypeJungle(int64_t seed){
  const StructureConfig sconf_portal = RUINED_PORTAL_CONFIG;
  int valid2, portalType;
  Pos p2 = getStructurePos(sconf_portal, seed, 0, 0, &valid2);
  unsigned long modulus = 1UL << 48;
  int portcx, portcz, portOceanType, portNormalY, portNormalType;
  float buriedFloat, bigOrSmall;
  int rawPortalType;
  int64_t fakeseed, carvea, carveb;
  portcx = p2.x >> 4;
  portcz = p2.z >> 4;
  fakeseed = (seed) ^ 0x5deece66dL;
  carvea = nextLong(&fakeseed);
  carveb = nextLong(&fakeseed);
  fakeseed = ((portcx * carvea) ^ (portcz * carveb) ^ seed) ^ 0x5deece66dL;
  fakeseed = fakeseed & 0xFFFFFFFFFFFF;
  next(&fakeseed, 31); //tossed out for jungle terrains (air gaps)
  //  buriedFloat = nextFloat(&fakeseed); // jungle never buries it
  bigOrSmall = nextFloat(&fakeseed); // 1/20 chance of being big
  rawPortalType = next(&fakeseed, 31); //once this is reduced mod 3 or 10 we know the type
  if (bigOrSmall < .05){
    return 1; //all three big ones have enough lava and we're not underground
  }
  portalType = rawPortalType % 10;
  if (portalType == 0 || portalType == 2 || portalType == 3 || portalType == 4 || portalType == 5 || portalType == 8){
    return 0; //6 small types have no lava
  }
  return 1; //this seed made it
}

int portalTypeOcean(int64_t seed){
  return 1; //in this case we're either buried or underwater so the lava doesn't matter
  //this is really just a chest and gold
  //IF the world demands portal Type filtering for the ocean/desert worlds I would do it here
}

int portalTypeNormal(int64_t seed){
  const StructureConfig sconf_portal = RUINED_PORTAL_CONFIG;
  int valid2, portalType;
  Pos p2 = getStructurePos(sconf_portal, seed, 0, 0, &valid2);
  int portcx, portcz, portOceanType, portNormalY, portNormalType;
  float buriedFloat, bigOrSmall;
  int rawPortalType;
  int64_t fakeseed, carvea, carveb;
  portcx = p2.x >> 4;
  portcz = p2.z >> 4;
  fakeseed = (seed) ^ 0x5deece66dL;
  carvea = nextLong(&fakeseed);
  carveb = nextLong(&fakeseed);
  fakeseed = ((portcx * carvea) ^ (portcz * carveb) ^ seed) ^ 0x5deece66dL;
  fakeseed = fakeseed & 0xFFFFFFFFFFFF;
  buriedFloat = nextFloat(&fakeseed); // 50/50 shot at being underground
  next(&fakeseed, 31); //tossed out for normal terrains (air gaps)
  bigOrSmall = nextFloat(&fakeseed); // 1/20 chance of being big
  rawPortalType = next(&fakeseed, 31); //once this is reduced mod 3 or 10 we know the type
  if (buriedFloat < .5){
    return -1;
  };

  int keyo[13][11][3] = {3,2,2,3,2,3,3,3,1,3,4,1,3,5,1,3,3,4,3,6,2,3,6,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,5,2,5,6,2,5,7,2,5,7,5,5,8,3,5,8,4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,4,3,3,4,3,4,4,4,5,4,5,5,4,6,5,4,7,4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,4,3,3,4,3,4,4,4,2,4,5,2,4,6,2,4,4,5,4,5,5,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,3,2,2,3,3,2,4,4,2,5,4,2,6,4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,2,1,1,2,1,2,2,1,3,2,2,0,2,2,4,2,3,0,2,3,4,2,4,0,2,4,4,2,5,1,2,5,3,3,0,3,3,0,4,3,1,2,3,2,2,3,3,2,3,1,5,3,2,5,3,4,3,3,4,4,-1,-1,-1,-1,-1,-1,5,3,3,5,3,4,5,3,5,5,4,2,5,4,6,5,5,2,5,5,6,5,6,2,5,6,6,-1,-1,-1,-1,-1,-1,4,1,4,4,1,5,4,2,3,4,2,6,4,3,6,4,4,6,4,5,4,4,5,5,-1,-1,-1,-1,-1,-1,-1,-1,-1,3,1,4,3,1,5,3,2,3,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,3,9,5,3,10,5,4,11,5,5,11,5,6,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,3,9,5,3,10,5,4,11,5,5,11,5,6,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,5,3,9,5,3,10,5,4,11,5,5,11,5,6,11,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
  //JUMP HERE
  int pdims[13][2] = {6, 6, 9, 9, 8, 9, 8, 9, 10, 7, 5, 7, 9, 9, 14, 9, 10, 9, 12, 10, 11, 16, 11, 16, 16, 16};
  if (bigOrSmall < .05){
    portalType =  10 + (rawPortalType % 3);//all three big ones have enough lava and we're not underground
  } else {
    portalType = rawPortalType % 10;
  }
  if (portalType != gtyp){
    return -1;
  }
  int rotv = nextInt(&fakeseed, 4);
  float mirv = nextFloat(&fakeseed);
  if (rotv != grot){
    return -1;
  }
  int mirb = (mirv < .5);
  if (mirb != gmir){
    return -1;
  }
  int yg = 0;
  int cryingcount = 0;
  int i,j,k;
  int signx, signz, conx, conz, cenx, cenz;
  int resx, resz, relx, relz, tempx, tempz, sizex, sizez;
  sizex = pdims[portalType][0];
  sizez = pdims[portalType][1];
  cenx = sizex >> 1;
  cenz = sizez >> 1;
  sizex = 0;
  sizez = 0;
  int flipper = 0;
  for(yg = 63; yg < 90; yg++){
    cryingcount = 0;
    for (i=0; i < 11; i++){
      if (keyo[portalType][i][0] > -1){
        relx = keyo[portalType][i][0];
        relz = keyo[portalType][i][2];
        if (gmir == 0){
          if (grot % 2 == 0){
            relx *= -1;
          } else if (grot %2 == 1){
            relz *= -1;
          }
        }

        relx -= cenx;
        relz -= cenz;

        tempx = relx;
        tempz = relz;
        if (grot == 1){
          relx = -tempz;
          relz = tempx;
        } else if (grot == 2){
          relx = -tempx;
          relz = -tempz;
        } else if (grot == 3){
          relx = tempz;
          relz = -tempx;
        }

        relx += cenx;
        relz += cenz;

        resx = p2.x + relx;
        resz = p2.z + relz;
        cryingcount += isCrying(resx, yg + keyo[portalType][i][1], resz);
        if (yg == 63){
          printf("(%d,%d+,%d) ", resx,yg + keyo[portalType][i][1],resz);
        }
      }
    }
    if (yg == 63){
      printf("\n");
    }
    printf("height %d: %d\n", yg, cryingcount);
  }
  return portalType;
/*  if (portalType == 0 || portalType == 2 || portalType == 3 || portalType == 4 || portalType == 5 || portalType == 8){
    return 0; //6 small types have no lava
  }
  return 1; //this seed made it*/
}

int strongholdAngle(int64_t seed, int fortressQuadrant){
  StrongholdIter sh;
  int mc = MC_1_16;
  Pos pos_sh = initFirstStronghold(&sh, mc, seed);
  long temp1, temp2, temp3;
  if (fortressQuadrant == -1){
    temp1 = l2norm(pos_sh.x, pos_sh.z, -1200L, 1200L);
    temp2 = l2norm(pos_sh.x, pos_sh.z, 1639L, 439L);
    temp3 = l2norm(pos_sh.x, pos_sh.z, -439L, -1639L);
    if ((temp1 > 300*300) && (temp2 > 300*300) && (temp3 > 300*300)){
      return 0;
    }
  }
  if (fortressQuadrant == 1){
    temp1 = l2norm(pos_sh.x, pos_sh.z, 1200L, -1200L);
    temp2 = l2norm(pos_sh.x, pos_sh.z, -1639L, -439L);
    temp3 = l2norm(pos_sh.x, pos_sh.z, 439L, 1639L);
    if ((temp1 > 300*300) && (temp2 > 300*300) && (temp3 > 300*300)){
      return 0;
    }
  }
  return 1;
}

int strongholdSlowCheck(int64_t seed, int fortressQuadrant, LayerStack* gp){
  StrongholdIter sh;
  int mc = MC_1_16;
  Pos pos_sh = initFirstStronghold(&sh, mc, seed);
  long sh_dist = 0xffffffffffff;
  long temp = 0;
  int i, N = 3;
  Pos best_sh;
  for (i = 1; i <= N; i++)
  {
    if (nextStronghold(&sh, gp, NULL) <= 0)
        break;
      if (fortressQuadrant == -1){
        temp = l2norm(sh.pos.x, sh.pos.z, -1200L, 1200L);
        if (temp < sh_dist){
          sh_dist = temp;
          best_sh  = sh.pos;
        }
      } else if (fortressQuadrant == 1){
        temp = l2norm(sh.pos.x, sh.pos.z, 1200L, -1200L);
        if (temp < sh_dist){
          sh_dist = temp;
          best_sh = sh.pos;
        }
      }
  }
  if (sh_dist > 300*300){
    return 0;
  }
  int shbiome = getBiomeAtPos(gp, best_sh);
  if (shbiome != deep_ocean && shbiome != deep_warm_ocean && shbiome != deep_lukewarm_ocean && shbiome != deep_cold_ocean && shbiome != deep_frozen_ocean){
    return 0;
  }
  return 1;
}

int valid_shipwreck_and_ravine_not_biome(int64_t lower48){
  if (ravinePositionAndQuality(lower48) == 0){
    return -7;
  }
  if (shipwreckLocationAndType(lower48) == 0){
    return -8;
  }
  return 1;
}

int valid_jungle_not_biome(int64_t lower48){
  if (jungleLocation(lower48) == 0){
    return -9;
  }
  if (portalTypeJungle(lower48) == 0){ //doing this now biome independent
    return -10;
  }
  return 1;
}

int portalLoot(int64_t lower48, int px, int pz, int pType){
  //here we GO
  //set decorator seed: worldSeed, portalPosition.getX() << 4, portalPosition.getZ() << 4, 40005
  int obiT[13] = {2, 4, 4, 3, 5,  1, 1, 3, 2, 7,   5, 5, 5};
  int obicutoff = 4;
  if (pType >= 0 && pType <= 12){
    obicutoff = obiT[pType];
  }
  int64_t fakeseed = (lower48) ^ 0x5deece66dUL;
  long a = nextLong(&fakeseed) | 1;
  long b = nextLong(&fakeseed) | 1;
  fakeseed = ((long)px * a + (long)pz * b) ^ lower48;
  //fakeseed = fakeseed ^ 0x5deece66dUL;
  fakeseed = fakeseed & 0xffffffffffff; //population seed set?
  int64_t cseed = (fakeseed + 40005) ^ 0x5deece66dUL;//decorator set?
  long chesti = nextLong(&cseed);
  cseed = (int64_t) chesti^ 0x5deece66dUL; //& 0xffffffffffff;// ^ 0x5deece66dUL;//?  IF THINGS ARE WRONG TRY THIS FIRST
  int num_rolls = 4 + nextInt(&cseed, 5);
  int ri = 0;
  int table[100] = {40, 1, 1, 2, 40, 1, 1, 4, 40, 1, 9, 18, 40, 0, 0, 0, 40, 0, 0, 0, 15, 0, 0, 0, 15, 1, 4, 24, 15, 2, 0, 0, 15, 2, 1, 0, 15, 2, 2, 0, 15, 2, 3, 0, 15, 2, 4, 0, 15, 2, 5, 0, 15, 2, 6, 0, 15, 2, 7, 0, 15, 2, 8, 0, 5, 1, 4, 12, 5, 0, 0, 0, 5, 0, 0, 0, 5, 1, 4, 12, 5, 0, 0, 0, 5, 1, 2, 8, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 2};
  int etable[45] = {10, 8, 9, -1, -1, 9, 4, 7, 8, -1, 6, 1, 4, 5, -1, 6, 1, 4, 5, -1, 6, 1, 4, 5, -1, 13, 8, 11, 12, -1, 9, 5, 7, 8, -1, 11, 5, 7, 9, 10, 9, 5, 7, 8, -1};
  int itemi, rv, r_class, i_count, e_roll;
  int enti = -1;
  int flintc = 0, obic = 0, ironc = 0, goldc = 0;//counts in nuggets
  int fireb = 0, finishb = 0, lootb = 0;
  int gaxe = 0;
  int carrots = 0;
  for(ri = 0; ri < num_rolls; ri++){
    rv = nextInt(&cseed, 398);
    //printf("rv: %d\n", rv);
    for(itemi = 0; rv > 0; ){
      rv = rv - table[4*itemi];
      if (rv >= 0){
        itemi++;
      }
    }
    
    if (itemi == 8){
      gaxe = 1;
    }

    //printf("I think item %d was itemi %d\n", ri, itemi);
    r_class = table[4*itemi + 1];
    if (r_class == 0){
      i_count = 1;
      if (itemi == 3 || itemi == 4){
        fireb = 1;
      }
      continue;
    }
    //printf("continue?\n");
    if (r_class == 1){
      i_count = table[4*itemi + 2] + nextInt(&cseed, table[4*itemi + 3]-table[4*itemi + 2] + 1);
      //printf("item %d class 1 count: %d\n", ri, i_count);
      if (itemi == 0){
        obic += i_count;
      }
      if (itemi == 1){
        flintc += i_count;
      }
      if (itemi == 2){
        ironc += i_count;
      }
      if (itemi == 6){
        goldc += i_count;
      }
      if (itemi == 19){
        carrots += i_count;
      }
      if (itemi == 21){
        goldc += 9*i_count;
      }
      if (itemi == 24){
        goldc += 81*i_count;
      }
      continue;
    }

    if (r_class == 2){
      enti = 5*table[4*itemi + 2];
      e_roll = nextInt(&cseed, etable[enti]);
      //printf("e_roll %d\n", e_roll);
      if (e_roll != etable[enti+1] && e_roll != etable[enti+2] && e_roll != etable[enti+3] && e_roll != etable[enti+4]){
        int elevel = nextInt(&cseed, 3) + 1;
        if (itemi == 7 && e_roll == 5 && elevel >= 2){
          lootb = 1;
        }
      }
    }
  }
  int fitness = 0;
  int axeeq=gaxe;
  if (goldc >= 27){
    axeeq = 1;
  }
  int ironic = ironc/9;//iron_ingot_count
  /*if (ironic % 3 != 0){
    if (fireb == 0 && flintc > 0){
      ironic -= 1;
      fireb = 1; //if you can build a flint and steel and it won't drop you out of bucket/pick range then do it
    }
  }*/
  printf("portal at %d %d\n",px,pz);
  printf("ironc: %d, obic: %d, flintc: %d, fireb: %d, lootb: %d, goldc: %d, gaxe: %d\n", ironc, obic, flintc, fireb, lootb, goldc, gaxe);
  return 1;
  if (lootb > 0 && obic >= obicutoff && fireb > 0 && axeeq > 0 && (carrots > 0 || ironic > 0)){
    fitness += 100;
  }
  if (fitness >= 80){ // around 5 is 3 iron and flint and steel, around 10 is that plus lots of OBI
    //
    //printf("oc %d/%d\n", obic, obicutoff);
    return 1;
  }    
  return 0;
}

int valid_village_and_portal_not_biome(int64_t lower48){
  int px,pz;
  if (portalLocation(lower48, 1, &px, &pz) == 0){ //village special portal
    return -3;
  }
  if (villageLocation(lower48) == 0){
    return -4;
  }
  if (portalTypeNormal(lower48) == 0){ //doing this now biome independent
    return -5;
  }
  if (portalLoot(lower48, px, pz, -1) == 0){
    return -777;
  }

  return 1; //has filtered nether for pos/pos bast and 1 fotress + rough stronghold blind + portal location (the L) + village location (square)
}

int possible_lava(int64_t lower48, int x, int z){
  //printf("lower48: %ld\n", lower48);
  int64_t fakeseed = (lower48) ^ 0x5deece66dUL;
  long a = nextLong(&fakeseed) | 1;
  long b = nextLong(&fakeseed) | 1;
  fakeseed = ((long)x * a + (long)z * b) ^ lower48;
  fakeseed = fakeseed & 0xffffffffffff; //population seed set?
  //printf("population seed: %ld\n", fakeseed);
  int64_t lakeseed = (fakeseed + 10000) ^ 0x5deece66dUL;
  //printf("lakeseed: %ld\n", lakeseed);
  int64_t lavaseed = (fakeseed + 10001) ^ 0x5deece66dUL;
  //printf("lavaseed: %ld\n", lavaseed);
  if (nextInt(&lakeseed, 4) != 0  && nextInt(&lavaseed, 8) != 0){
    return 0;
  }
  nextInt(&lakeseed, 16); //noise in X
  nextInt(&lakeseed, 16); //noise in Z
  nextInt(&lavaseed, 16); //noise in X
  nextInt(&lavaseed, 16); //noise in Z
  
  int lakey = nextInt(&lakeseed, 256);
  int temp = nextInt(&lavaseed, 256 - 8);
  int lavay = nextInt(&lavaseed, temp + 8);

  if (nextInt(&lavaseed, 10) != 0){
    return 0;
  }

  if (lavay > 63 && lakey < 63){
    //printf("lavay %d, lakey %d\n", lavay, lakey);
    return 1;
  }
  return 0;
}

int lava_grid(int64_t lower48){
  int lava_count = 0;
  int cx,cz;
  for(cx = -4; cx < 10; cx++){
    for(cz = -4; cz < 10; cz++){
      lava_count += possible_lava(lower48, cx<<4, cz<<4);
    }
  }
  if (lava_count < 1){
    return 0;
  }
  return 1;
}

int lava_biome(int64_t seed, int x, int z, LayerStack* gp){
  Pos spawn;
  spawn.x = x + 8;
  spawn.z = z + 8;
  int lavabiome = getBiomeAtPos(gp, spawn);
  if (lavabiome == desert){
    return 1;
  }
  return 0;
}

int portalPreLoot(int64_t seed, int* px, int* pz, int* bigsmall, int* pType){
  const StructureConfig sconf_portal = RUINED_PORTAL_CONFIG;
  int valid2;
  Pos p2 = getStructurePos(sconf_portal, seed, 0, 0, &valid2);
  if (!valid2){// || p2.x > 32 || p2.z > 32){
    return 0;
  }
  int portalType;
  int portcx, portcz, portOceanType, portNormalY, portNormalType;
  float buriedFloat, bigOrSmall;
  int rawPortalType;
  int64_t fakeseed, carvea, carveb;
  portcx = p2.x >> 4;
  portcz = p2.z >> 4;
  fakeseed = (seed) ^ 0x5deece66dL;
  carvea = nextLong(&fakeseed);
  carveb = nextLong(&fakeseed);
  fakeseed = ((portcx * carvea) ^ (portcz * carveb) ^ seed) ^ 0x5deece66dL;
  fakeseed = fakeseed & 0xFFFFFFFFFFFF;
  buriedFloat = nextFloat(&fakeseed); // 50/50 shot at being underground
  next(&fakeseed, 31); //tossed out for normal terrains (air gaps)
  bigOrSmall = nextFloat(&fakeseed); // 1/20 chance of being big
  rawPortalType = next(&fakeseed, 31); //once this is reduced mod 3 or 10 we know the type
  if (buriedFloat < .5){
    return 0;
  }
  if (bigOrSmall < .05){
    *bigsmall = 1; //all three big ones have enough lava and we're not underground
    *pType = rawPortalType % 3;
  } else {
    *bigsmall = 0;
    *pType = rawPortalType % 10;
    portalType = rawPortalType % 10;
    if (portalType == 0 || portalType == 2 || portalType == 3 || portalType == 4 || portalType == 5 || portalType == 8){
      return 0; //6 small types have no lava
    }
  }
  *px = p2.x;
  *pz = p2.z;
  //printf("portal at /tp @p %d ~ %d and is of type: %d\n", *px, *pz, *pType);
  return 1;
}

int valid_structures_and_types2(int64_t lower48, int* fortressQuadrant, int filter_style, int* px2, int* pz2){
  /*if (netherchecker(lower48, fortressQuadrant) == 0){
    return -1;
  }
  if (strongholdAngle(lower48, *fortressQuadrant) == 0){
    return -2;
  }*/
  if (portalLocation(lower48, 0, px2, pz2) == 0){ //for all future filters
    return -6;
  }
  int portalStyle = portalTypeNormal(lower48);
  if (portalStyle == -1){ //doing this now biome independent
    return -5;
  }
  /*if (portalLoot(lower48, *px2, *pz2, portalStyle) == 0){
    return -777;
  }*/
  return 1;
}

int valid_structures_and_types(int64_t lower48, int* fortressQuadrant, int filter_style){
  if (filter_style == 6){
    int px,pz,bigsmall,pType;
    if (portalPreLoot(lower48, &px, &pz, &bigsmall, &pType) == 0){
      return 0;
    }
    if (portalLoot(lower48, px, pz, -1) == 0){
      return 0;
    }
    return 1;
  }

  if (netherchecker(lower48, fortressQuadrant) == 0){
    return -1;
  }

  if (strongholdAngle(lower48, *fortressQuadrant) == 0){
    return -2;
  }

  if (filter_style == 0){
    int px2, pz3;
    if (portalLocation(lower48, 0, &px2, &pz3) == 0){ //for all future filters
      return -6;
    }
    int portalStyle = portalTypeNormal(lower48);
    if (portalStyle == -1){ //doing this now biome independent
      return -5;
    }
    if (portalLoot(lower48, px2, pz3, portalStyle) == 0){
      return -777;
    }
   
    return 1; //good nether, good blind, no overworld
  }

  if (filter_style == 1){//village only
    /*if (lava_grid(lower48) == 0){
      return -666;
    }*/
    //filters portal in L village in square and normal portal type
    return valid_village_and_portal_not_biome(lower48); //farmed out so we could do pre-planned or either
  }

  int px, pz;
  if (portalLocation(lower48, 0, &px, &pz) == 0){ //for all future filters
    return -6;
  }

  if (filter_style == 2){//shipwreck
    if (portalTypeNormal(lower48) == 0){ //doing this now biome independent
      return -5;
    }
    if (portalLoot(lower48, px, pz, -1) == 0){
      return -777;
    }
    return valid_shipwreck_and_ravine_not_biome(lower48);
  }

  if (filter_style == 3){//jungle only
    return valid_jungle_not_biome(lower48);
  }
  return 1;
}

int spawn_close2(int64_t seed, LayerStack* gp, int x, int z){ //costs about 1/6 biomes...
  int mc = MC_1_16;
  Pos spawn = getSpawn(mc, gp, NULL, seed);
  long targetD = l2norm(x, z, spawn.x, spawn.z);
  if (targetD <= 90*90){
    return 1;
  }
  return 0;
}

int spawn_close(int64_t seed, LayerStack* gp){ //costs about 1/6 biomes...
  int mc = MC_1_16;
  Pos spawn = getSpawn(mc, gp, NULL, seed);
  if (spawn.x >= -32 && spawn.x <= 80 && spawn.z >= -32 && spawn.z <= 80){
    return 1;
  }
  return 0;
}

int spawn_medium(int64_t seed, LayerStack* gp){ //for shipwrecks we're ok being a little farther away for trees
  int mc = MC_1_16;
  Pos spawn = getSpawn(mc, gp, NULL, seed);
  if (spawn.x >= -100 && spawn.x <= 200 && spawn.z >= -100 && spawn.z <= 200){
    return 1;
  }
  return 0;
}

int villageBiome(int64_t seed, LayerStack* gp){
  const StructureConfig sconf = VILLAGE_CONFIG;
  int valid;
  Pos p = getStructurePos(sconf, seed, 0, 0, &valid);
  int mc = MC_1_16;
  if (!isViableStructurePos(sconf.structType, mc, gp, seed, p.x, p.z)){
    return 0;
  }
  int biome = getBiomeAtPos(gp, p);
  if (biome == snowy_tundra || biome == desert){
    return 0;
  }

  return 1;
}

int shipwreckBiome(int64_t seed, LayerStack* gp){
  const StructureConfig sconf_shipwreck = SHIPWRECK_CONFIG;
  int valid3;
  Pos p3 = getStructurePos(sconf_shipwreck, seed, 0, 0, &valid3);
  int mc = MC_1_16;
  if (!isOceanic(getBiomeAtPos(gp, p3))){
    return 0;
  }
  if (!isViableStructurePos(sconf_shipwreck.structType, mc, gp, seed, p3.x, p3.z)){
    return 0;
  }
  return 1;
}

int jungleBiome(int64_t seed, LayerStack* gp){
  const StructureConfig sconf = JUNGLE_PYRAMID_CONFIG;
  int valid;
  Pos p = getStructurePos(sconf, seed, 0, 0, &valid);
  int mc = MC_1_16;
  if (!isViableStructurePos(sconf.structType, mc, gp, seed, p.x, p.z)){
    return 0;
  }
  return 1;
}

int valid_biomes2(int64_t seed, int* fortressQuadrant, int filter_style, LayerStack* gp, int px3, int pz3){
  applySeed(gp, seed);
  /*if (strongholdSlowCheck(seed, *fortressQuadrant, gp) == 0){
    return -20;
  }*/
  if (spawn_close2(seed, gp, px3, pz3) == 0){
    return -11;
  }
  if (portalBiome(seed, gp)==0){
    return -17;
  }
  return 1; 
}

int valid_biomes(int64_t seed, int* fortressQuadrant, int filter_style, LayerStack* gp){
  applySeed(gp, seed);
  if (filter_style == 6){

    return 1;
  }

  if (strongholdSlowCheck(seed, *fortressQuadrant, gp) == 0){
    return -20;
  }
  if (filter_style == 0){
    if (spawn_close(seed, gp) == 0){
      return -11;
    }

    if (portalBiome(seed, gp)==0){
      return -17;
    }
    return 1;
  }

  if (portalBiome(seed, gp)==0){
    return -17;
  }
  if (filter_style == 1){
    if (spawn_close(seed, gp) == 0){
      return -11;
    }
    if (villageBiome(seed, gp)== 0){
      return -12;
    }
  }

  if (filter_style == 3){
    if (spawn_close(seed, gp) == 0){
      return -11;
    }
    if (jungleBiome(seed, gp)== 0){
      return -13;
    }
  }

  if (filter_style == 2){
    if (shipwreckBiome(seed, gp)== 0){
      return -14;
    }
    if (spawn_close(seed, gp) == 0){
      return -15;
    }
    if (ravineBiome(seed, gp) == 0){
      return -16;
    }
  }
  return 1;
}

int main (int argc, char *argv[]) {
    int64_t seed;
    if (argc == 2){
      seed = atol(argv[1]);
    } else {
      seed = 1L;
    }
    int px,pz;
    portalLocation(seed, 5, &px, &pz);
    portalLoot(seed, px, pz, 9);
    return 0;
}