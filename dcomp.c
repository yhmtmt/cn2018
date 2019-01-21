#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "dcomp.h"

int main(int argc, char **argv)
{
  int i, j, k, l;
  FILE * f; // File to be compressed
  unsigned char c, len_code, len_null, * data;
  unsigned int len_bits, len_bytes, sz_org, sz_cmp, nhnodes;
  struct hnode leaf[NSYMBOLS], * htop[NSYMBOLS], * htmp;
  struct hcode code[NSYMBOLS];
  struct hnode_save * htree_dec;
  float h, p;
  
  if (argc != 3){
    printf("Usage: dcomp <src file> <cmpd file>\n");
    return 1;
  }
  
  f = fopen(argv[1], "rb");
  
  if(f == NULL){
    printf("Error in opening file %s\n", argv[1]);
    return 1;
  }

  // calculate symbol frequency 
  for (i = 0; i < NSYMBOLS; i++){
    leaf[i].u = leaf[i].p = leaf[i].n = NULL;
    leaf[i].frq = 0;
  }

  sz_org = 0;
  while(fread(&c, sizeof(char), 1, f)){
    leaf[c].frq++;
    sz_org++;
  }
  fseek(f, 0, SEEK_SET);

  // calculate entropy (only for indication)
  print_entropy(leaf);
  
  // building Huffman tree
  // 1. set top nodes as leaf nodes
  for (i = 0; i < NSYMBOLS; i++)
    htop[i] = &leaf[i];

  // 2. sort leaf nodes by their symbol frequency 
  for (j = 0; j < NSYMBOLS; j++){
    for(k = 1; k < NSYMBOLS - j; k++){
      if(htop[k-1]->frq < htop[k]->frq){
	htmp = htop[k-1];
	htop[k-1] = htop[k];
	htop[k] = htmp;
      }
    }
  }

  //3. combine two least frequent nodes with a parent node,
  //   replace them with the parent, and sort.
  //   Finally we get an unique root node htop[0]
  for(i = 1; i < NSYMBOLS; i++){
    htmp = (struct hnode*) malloc(sizeof(struct hnode));
    htmp->u = NULL;
    htmp->p = htop[NSYMBOLS - i - 1];
    htmp->n = htop[NSYMBOLS - i];
    htmp->frq = htmp->p->frq + htmp->n->frq;
    htmp->p->u = htmp->n->u = htmp;
    
    htop[NSYMBOLS - i - 1] = htmp;
    htop[NSYMBOLS - i] = NULL;
    for (j = NSYMBOLS - i - 1; j > 0; j--){
      if(htop[j - 1]->frq <  htop[j]->frq){
	htmp = htop[j - 1];
	htop[j - 1] = htop[j];
	htop[j] = htmp;
      }else
	break;
    }
  }  

  // build symbol' huffman codes 
  for (i = 0; i < NSYMBOLS; i++){
    code[i].len = 0;
    code[i].code = NULL;
    set_code(&leaf[i], NULL, &code[i], 0);
  }

  // calculate resulting bit length, then prepare buffer
  len_bits = 0;
  for (i = 0; i < NSYMBOLS; i++){
    len_bits += code[i].len * leaf[i].frq;
  }
  len_null = len_bits % 8;
  len_bytes = len_bits / 8 + (len_null == 0 ? 0 : 1);
  
  printf("Estimated result is %d bytes (%d bits)\n",
	 len_bytes, len_bits);
  
  data = (char*)malloc(len_bytes);
  for (i = 0; i < len_bytes; i++){
    data[i] = 0x00;
  }
  
  if (data == NULL){
    printf("Error. Failed to allocate data buffer.\n");
    return 1;
  }
  
  // Encode with huffman code
  i = j = 0; // using (i, j) as pointer to a bit
            // i: byte count, j: bit count (0-7)

  while(fread(&c, sizeof(char), 1, f)){
    // len_code 1s are sat
    k = l = 0;
    for (len_code = code[c].len;len_code > 0; len_code--){
      data[i] |= (code[c].code[k] & (0x01 << l) ? 0x01 << j : 0);

      // increment data pointer (i, j)
      j++;
      if(j == 8){
	i++;
	j = 0;
      }

      // increment code pointer (k, l)
      l++;
      if(l == 8){
	k++;
	l = 0;
      }
    }
  }

  fclose(f);
  
  // building Huffman tree for decode program
  nhnodes = count_hnode(htop[0]);
  htree_dec = (struct hnode_save *) malloc(nhnodes * sizeof(struct hnode_save));
  for(i = 0; i < NSYMBOLS; i++){
    htree_dec[i].p = htree_dec[i].n = -1;
    leaf[i].frq = (unsigned int)i; // nusty hack
  }
  build_htree_dec(htop[0], htree_dec, nhnodes);
  
  // write
  f = fopen(argv[2], "wb");
  if(f == NULL){
    printf("Error in opening file %s\n", argv[2]);
    return 1;
  }
  
  sz_cmp = fwrite(&len_null, sizeof(char), 1, f);
  sz_cmp += fwrite(&nhnodes, sizeof(nhnodes), 1, f);
  sz_cmp += fwrite(&len_bytes, sizeof(len_bytes), 1, f);
  sz_cmp += fwrite(htree_dec, sizeof(struct hnode_save), nhnodes, f);
  sz_cmp += fwrite(data, sizeof(char), len_bytes, f);
  
  fclose(f);
  
  free((void*) htree_dec);
  
  printf("Data compression done.\n");
  printf("Original file is %d bytes, Compressed file is %d bytes\n",
	 sz_org, sz_cmp);
  printf("Data compression rate is %1.3f\n",
	 (float)((float)sz_cmp / (float)sz_org));

  
  // release code
  free((void*) data);
  for (i = 0; i < NSYMBOLS; i++){
    free((void*)code[i].code);
  }

  free_htree(htop[0]);
  return 0;
}
