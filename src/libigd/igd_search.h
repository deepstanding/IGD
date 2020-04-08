#ifndef __IGD_SEARCH_H__
#define __IGD_SEARCH_H__

#ifdef __cplusplus
extern "C"{
#endif 

//=====================================================================================
//Search the igd database for all overlaps with queries
//by Jianglin Feng  05/12/2018
//
//database intervals sorted by _start: 8/12/2019
//Reorganized: 11/06/2019
//R wrapper 12/06/2019
//-------------------------------------------------------------------------------------
#include "igd_base.h"
//-------------------------------------------------------------------------------------

//Hashtable for R
typedef struct htNode {                           //linked list item
    char *key;
    int32_t value;
    struct htNode *next;
} htNode;

typedef struct {
    int32_t size;
    htNode **nodes;
} hTable;

typedef struct {
    int32_t nbp, gType, nCtg, nFiles;
    int32_t preIdx, preChr;
    //char fname[64];                   //igdFile
    char **cName;                       //name of ctgs
    int32_t *nTile;                     //num of tiles in each ctg
    int32_t **nCnt;                     //num of counts in each tile
    int64_t **tIdx;                     //tile index *sizeof -> location in .igd file
    //gdata_t *gData;
    //info_t *finfo;
    //FILE *fP;
    //void *hc;
} igdr_t;

int32_t hash(const char *key, int32_t htsize);     //hash function
hTable *ht_init(int32_t htsize);                   //initialize
int ht_put(hTable *ht, const char *key, int32_t value);
int32_t ht_get(hTable *ht, const char *key);
void ht_free(hTable* ht);

//Single query
void get_overlaps(IGD_t *IGD, char *chrm, int32_t qs, int32_t qe, int64_t *hits);

//32bit
void get_overlaps32(IGD_t *IGD, char *chrm, int32_t qs, int32_t qe, int32_t *hits);

void search_1(char **igdFile, char **qchr, int32_t *qs, int32_t *qe, int64_t *hits);


/**
 * @brief Compute overlaps between a query file and IGD database
 *
 * @param IGD The database object
 * @param qFile Path to query bed file
 * @param hits Pre-allocated vector of length IGD->nFiles, for returned results
 * @return A vector of overlap counts for each database file.
 */
int32_t* getOverlapsFile(IGD_t* IGD, char *qFile, int32_t *hits);

/**
 * @brief Compute overlaps between a query Vector and IGD database
 *
 * @param IGD The database object
 * @param qLength Number of elements in the query vectors (which should be the same)
 * @param qChrs Vector of chromosome identifiers 
 * @param qStarts Vector of start coordinates
 * @param qEnds Vector of end coordinates 
 * @param hits Pre-allocated vector of length IGD->nFiles, for returned results
 * @return A vector of overlap counts for each database file.
 */
int32_t* getOverlapsVectors(IGD_t* IGD, int qLength, char **qChrs, 
    int32_t *qStarts, int32_t *qEnds, int32_t *hits);


#ifdef __cplusplus
}
#endif



#endif
