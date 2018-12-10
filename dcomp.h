#define NSYMBOLS 0xff
struct hnode{
  unsigned int frq;
  struct hnode * u, * p, * n;
};

struct hcode{
  int len;
  char * code;
};

struct hnode_save
{
  short p, n;
};

void print_entropy(struct hnode * leaf)
{
  int i;
  float p, h;
  unsigned ntotal = 0;
  for (i = 0; i < NSYMBOLS; i++){
    ntotal += leaf[i].frq;
  }

  h = 0.0f;
  for (i = 0; i < NSYMBOLS; i++){
    if(leaf[i].frq == 0)
      continue;
    p = (float)((float) leaf[i].frq / (float) ntotal);
    h += -p * log2(p);
  }
  printf("Entropy: %2.2f\n", h);
}

unsigned int count_hnode(struct hnode * htop)
{
  int num = 1;
  if(htop->p)
    num += count_hnode(htop->p);
  if(htop->n)
    num += count_hnode(htop->n);
  return num;
}

void free_htree(struct hnode * htop)
{ 
  if(htop->p == NULL && htop->n == NULL){ // leaf node is in stack.
    htop->u = NULL;
    htop->frq = 0;
    return;
  }
  
  free_htree(htop->p);
  free_htree(htop->n);
  free((void*)htop);
}

int set_code(struct hnode * pnode, struct hnode * pcnode,
	      struct hcode * pcode, int len)
{
  int i, j;
  if(pnode->u){
    pcode->len++;
    set_code(pnode->u, pnode, pcode, pcode->len);
  }else{// alloc code
    pcode->code = (char*) malloc((len / 8) + 1);
    for (i = 0; i < (len / 8) + 1; i++)
      pcode->code[i] = 0;
  }

  if(len > 0){
    i = (pcode->len - len) / 8;
    j = (pcode->len - len) % 8;
    if(pnode->p == pcnode){ // assign 1
      pcode->code[i] |= (0x01 << j);
    }
  }
}

char decode(struct hnode_save * htree_dec, int inode, char * data,
	    int * i, int * j)
{
  char bit;
  
  if(htree_dec[inode].n < 0){ // leaf node
    return (char) htree_dec[inode].p;
  }
  
  bit = data[*i] & (0x01 << *j);
  (*j)++;
  if(*j == 8){
    (*i)++;
    *j = 0;
  }      
  
  if(bit != 0){
    return decode(htree_dec, htree_dec[inode].p, data, i, j);
  }else{
    return decode(htree_dec, htree_dec[inode].n, data, i, j);    
  }  
}

void print_htree_dec(struct hnode_save * htree_dec, unsigned short nhnodes)
{
  int i;
  for (i = 0; i < (int)nhnodes; i++){
    printf("(%d:%d:%d)", i, (int)htree_dec[i].p, (int)htree_dec[i].n);
  }
  printf("\n");
}

unsigned short build_htree_dec(struct hnode * htop,
				struct hnode_save * htree_dec,
				unsigned short nhnodes)
{
  unsigned short inode = nhnodes - 1;

  if(htop->p){
    if(htop->p->p == NULL && htop->p->n == NULL){
      htree_dec[inode].p = htop->p->frq;
    }else{
      htree_dec[inode].p = nhnodes - 2;
      nhnodes = build_htree_dec(htop->p, htree_dec, nhnodes - 1);
    }
  }
  
  if(htop->n){
    if(htop->n->p == NULL && htop->n->n == NULL){
      htree_dec[inode].n = htop->n->frq;
    }else{
      htree_dec[inode].n = nhnodes - 2;
      nhnodes = build_htree_dec(htop->n, htree_dec, nhnodes - 1);
    }
  }
  
  return nhnodes;
}



