#pragma once
/*!
 * \file muloth.h
 * Describes the data structure *Grouped l-Othello*.
 * */
#include "othello.h"
#include "io_helper.h"
#include <cstdio>
#include <cstdlib>

#include <cstring>
using namespace std;
const char * VERSION = GITVERSION;
//#pragma message "VERSION: " GITVERSION

// split(k,plow,phigh,spl) (((plow)=(k & ((1ULL << spl)-1))), ((phigh)=(k >> spl)))

/*!
 * \brief Describes the data structure *Grouped l-Othello*. It classifies keys of *keyType* into *2^L* classes. \n
 * The keys are first classifed into groups, each group is then maintained by one *l-Othello*. 
 * \note Query a key of keyType return *L* digit values ( *valueIntType* ). the types *valueType* and *valueIntType* are automatically generated.
 */
template <uint8_t L, typename keyType>
class MulOth {
    vector<Othello<L, keyType> *> vOths; //!< list of *l-Othellos*
//!\cond typedef
#include "typedefine.h"
//!\endcond    
    unsigned char split;
    bool addOth(vector<keyType> &keys, vector<valueType> &values) {
        Othello<L, keyType> *poth;
        poth = new Othello<L,keyType>(&keys[0], &values[0], keys.size());
        if (!poth->build) {
            printf("Build Halt!\n");
            return false;
        }
        vOths.push_back(poth);
        return true;
    }
public:
    bool buildsucc; 
    /*!
     * \brief Construct a Grouped l-Othello from a file.
     * \param *fname, the file contains key/value pairs, each in a line.
     * \param _split, the keys are first classifed according to their highest *_split* bits. There are in total 2^_split groups. Classification method as described in *splitgrp*.
     * \param fileIsSorted. When fileIsSorted, assume that the file is sorted so that the keys appear in the ascending order of groups.
     * */
    MulOth(const char * fname, unsigned char _split, bool fileIsSorted = false) {
        printf("Building Multi Othello from file %s \n",fname);
        FILE *pFile;
        pFile = fopen (fname,"r");
        vOths.clear();
        char buf[1024];
        split = _split;
        if (split ==0) {
            keyType k; valueType v; 
            vector<keyType> keys;
            vector<valueType> values;
            while (true) {
                if (fgets(buf,1024,pFile)==NULL) break;
                if (!lineToKVpair(buf, &k, &v)) break;
                keys.push_back(k);
                values.push_back(v);
            }
            if (!addOth(keys,values)) return;
            buildsucc = true;
            return;
        }
        if (fileIsSorted)  {
            uint32_t grpid = 0;
            printf("Reading file for keys in group %02x/%02x\n", grpid,(1<<split)-1);
            vector<keyType> keys;
            vector<valueType> values;
            while (true) {
                keyType k; valueType v;
                void * pp;
                if (fgets(buf,1024,pFile)==NULL) break;
                if (!lineToKVpair(buf, &k, &v)) break;
                keyType keyingroup;
                uint32_t groupid;
                splitgrp(k,groupid,keyingroup,split);
                if (groupid != grpid) {
                    if (!addOth(keys,values)) return;
                    grpid++;
                    printf("Reading file for keys in group %02x/%02x\n", grpid,(1<<split)-1);
                    keys.clear();
                    values.clear();
                }
                keys.push_back(keyingroup);
                values.push_back(v);
            }
            if (!addOth(keys,values)) return;
        }
        else
            for (uint32_t grpid = 0; grpid < (1<<_split); grpid++) {
                printf("Reading file for keys in group %02x/%02x\n", grpid,(1<<split)-1);
                vector<keyType> keys;
                vector<valueType> values;
                rewind(pFile);
                while (true) {
                    keyType k;
                    valueType v;
                    if (fgets(buf,1024,pFile)==NULL) break;
                    if (!lineToKVpair(buf, &k, &v)) break;
                    keyType keyingroup;
                    uint32_t groupid;
                    splitgrp(k,groupid,keyingroup,split);
                    if (groupid != grpid) continue;
                    keys.push_back(keyingroup);
                    values.push_back(v);
                }
                printf("keycount %d ", keys.size());
                if (!addOth(keys,values)) return;
            }

        buildsucc = true;

    }

    /*!
       \brief returns a *L*-bit integer query value for a key.
       \note enabled when L is in {1,2,4,8,16,32,64}.
      */
    inline valueIntType query(const keyType &k) {
        uint32_t grp;
        keyType kgrp;
        if (split ==0) 
            return vOths[0]->queryInt(k);
        else {
            splitgrp(k,grp,kgrp,split);
            return vOths[grp]->queryInt(kgrp);
        }
    }
    void printall () {
        printf("Printall ...\n");
        for (auto V : vOths)
            V.printall();
    }
    //! \brief write Grouped l-Othello to a file.
    void writeToFile(const char* fname) {
        FILE *pFile;
        pFile = fopen (fname, "wb");
        unsigned char buf0x20[0x20];
        char *p;
        p = (char *) &(buf0x20[0]);
        memset(buf0x20,0, sizeof(buf0x20));
        strcpy(p+0x4,VERSION);
        uint32_t split32 = split;
        memcpy(buf0x20, &split32, sizeof(uint32_t));
        fwrite(buf0x20,sizeof(buf0x20),1,pFile);
        for (int i = 0; i <(1<<split); i++) {
            vOths[i]->exportInfo(buf0x20);
            fwrite(buf0x20,sizeof(buf0x20),1,pFile);
        }
        for (int i = 0; i <(1<<split); i++) {
            fwrite(&(vOths[i]->mem[0]),sizeof(vOths[i]->mem[0]), vOths[i]->mem.size(), pFile);
        }
        fclose(pFile);
    }

    // \brief construct a Grouped l-Othello from a file.
    MulOth(const char* fname) {
        buildsucc = false;
        printf("Read from binary file %s\n", fname);
        FILE *pFile;
        pFile = fopen (fname, "rb");
        uint32_t compversion;
        unsigned char buf0x20[0x20];
        fread(buf0x20,sizeof(buf0x20),1,pFile);
        char *p;
        p = (char *) &(buf0x20[0]);
#ifndef NO_VERSION_CHECK
        if (strcmp(p+4,VERSION)) {
            printf("Wrong version number\n");
            fclose(pFile);
            return;
        }
#endif
        uint32_t split32;
        memcpy(&split32, buf0x20, sizeof(uint32_t));
        split = split32;
        for (int i = 0; i < (1<<split); i++) {
            fread(buf0x20,sizeof(buf0x20),1,pFile);
            Othello<L,keyType> *poth;
            poth = new Othello<L,keyType>(buf0x20);
            vOths.push_back(poth);
        }
        for (int i = 0; i < (1<< split); i++) {
            fread(&(vOths[i]->mem[0]),sizeof(vOths[i]->mem[0]), vOths[i]->mem.size(), pFile);
        }
        fclose(pFile);
        buildsucc = true;
    }
};


//MulOthello binary file descriptor
//0x00 : uint32_t splitbit
//0x04 : uint32_t signature MulOth version
//0x20 : OthInfo1
//0x40 : OthInfo2
//...
//offset1 : Oth[0].m
//offset2 = offset1 + Oth[0].hashupperbound = Oth[1].m
//...

//OthInfo: 32 Byte
//0x00 : uint64_t hash1
//0x08 : uint64_t hash2
//0x10 : uint32_t mask1
//0x14 : uint32_t mask2
//0x18 : uint64_t m.offset

