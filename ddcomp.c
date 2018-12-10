#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dcomp.h"


int main(int argc, char ** argv)
{
  int i, j;
  char len_null, * data, c;
  unsigned int sz_org, sz_dcmp, nhnodes, len_bytes;
  struct hnode_save * htree_dec;
  FILE * f;

  if(argc != 3){
    printf("Usage: ddcomp <src file> <dcmpd file>\n");
    return 1;
  }

  // read data
  f = fopen(argv[1], "rb");

  if (f == NULL){
    printf("Error in opening file %s\n", argv[1]);
    return 1;
  }

  sz_org = fread(&len_null, sizeof(char), 1, f);
  sz_org += fread(&nhnodes, sizeof(nhnodes), 1, f);
  sz_org += fread(&len_bytes, sizeof(len_bytes), 1, f);
  htree_dec = (struct hnode_save*) malloc(nhnodes * sizeof(struct hnode_save));
  sz_org += fread(htree_dec, sizeof(struct hnode_save), nhnodes, f);
  for(i = 0; i < NSYMBOLS; i++){
    htree_dec[i].p = i; // nusty hack
    htree_dec[i].n = -1;
  }

  data = (char*) malloc(len_bytes);
  sz_org += fread(data, sizeof(char), len_bytes, f);

  fclose(f);
    
  // decompress data
  f = fopen(argv[2], "wb");

  if(f == NULL){
    printf("Error in opening file %s\n", argv[2]);
    return 1;
  }

  i = j = 0;
  sz_dcmp = 0;
  while(i < len_bytes){
    c = decode(htree_dec, nhnodes - 1, data, &i, & j);
    sz_dcmp += fwrite(&c, sizeof(char), 1, f);
    if(i == len_bytes - 1 && len_null == j){
      break;
    }
  }
  
  fclose(f);
  
  return 0;
}
