/* cJSON */
/* JSON parser in C. YAML output */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"
#include "base64.h"

#define  cJSON_strdup  strdup
#define  cJSON_malloc  malloc
#define  cJSON_free    free

/* Predeclare these prototypes. */
static char *print_number(cJSON *item);
static char *print_value(cJSON *item,int depth);
static char *print_array(cJSON *item,int depth);
static char *print_object(cJSON *item,int depth);

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str)
{
	const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
  int quote = 0;
	
	if (!str) return cJSON_strdup("");
	ptr=str;
  while ((token=*ptr) && ++len) {
    if (strchr("\"\\\b\f\n\r\t",token)){ 
      len++; quote = 1;
    } else if (token<32){ len+=5; quote = 1; }
    ptr++;
  }

	out=(char*)cJSON_malloc(len+3);
	if (!out) return 0;

	ptr2=out;ptr=str;
	if( quote ) *ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
			}
		}
	}
	if( quote )*ptr2++='\"';
  *ptr2++=0;
	return out;
}

static char *print_string(cJSON *item)	{
  return print_string_ptr(item->valuestring);
}

/* Render an array to text */
static char *print_array(cJSON *item,int depth)
{
	char **entries;
	char *out=0,*ptr,*ret;int len=5;
	cJSON *child=item->child;
	int numentries=0,i=0,fail=0,j;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Explicitly handle numentries==0 */
	if (!numentries)
	{
		out=(char*)cJSON_malloc(3);
		if (out) strcpy(out,"[]");
		return out;
	}
	/* Allocate an array to hold the values for each */
	entries=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	memset(entries,0,numentries*sizeof(char*));
	/* Retrieve all the results: */
	child=item->child;
	while (child && !fail)
	{
		ret=print_value(child,depth+1);
		entries[i++]=ret;
		if (ret) len+=strlen(ret)+2+(depth*3); else fail=1;
		child=child->next;
	}
	
	/* If we didn't fail, try to malloc the output string */
	if (!fail) out=(char*)cJSON_malloc(len);
	/* If that fails, we fail. */
	if (!out) fail=1;

	/* Handle failure. */
	if (fail)
	{
		for (i=0;i<numentries;i++) if (entries[i]) cJSON_free(entries[i]);
		cJSON_free(entries);
		return 0;
	}
	
	/* Compose the output array. */
	ptr=out;*ptr=0;
	for (i=0;i<numentries;i++)
	{
    *ptr++= '\n';
	  for (j=0;j<depth;j++){ *ptr++=' '; *ptr++=' '; };
    *ptr++= '-'; *ptr++ = ' ';
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		cJSON_free(entries[i]);
	}
	cJSON_free(entries);
	*ptr++=0;
	return out;	
}

static char *print_binary(cJSON *item, int depth){
  char* b64;
  char* out, *prt, *prt2, *prt3;
  int size=base64_encode_alloc( item->valuedata, item->valuesize, &b64 );
  int lines = size / 50 + 1;
  int len = lines  * ( depth * 2 + 6 + 51 ) + 11;
  out = cJSON_malloc( len );
  prt = out;
  *prt = 0;
  strcpy( prt, "!!binary |\n" );
  prt += 11;
  prt3 = b64 + size;
  prt2 = b64;
  while( prt2 < prt3 ){
    int i;
    for( i = 0; i < depth * 2 + 2; i ++ ){ *prt++ = ' '; };
    int resto = prt3 - prt2 > 50 ? 50 : prt3 - prt2;
    memcpy( prt, prt2, resto );
    prt2 += 50;
    prt += resto;
    *prt ++ = '\n';
  }
  *prt++ = 0;

  free( b64 );
  return out;

}


/* Render a value to text. */
static char *print_value(cJSON *item,int depth)
{
	char *out=0;
	if (!item) return 0;
	switch ((item->type)&255)
	{
		case cJSON_NULL:	out=cJSON_strdup("");	break;
		case cJSON_False:	out=cJSON_strdup("false");break;
		case cJSON_True:	out=cJSON_strdup("true"); break;
		case cJSON_Number:	out=print_number(item);break;
		case cJSON_String:	out=print_string(item);break;
		case cJSON_Array:	out=print_array(item,depth);break;
		case cJSON_Object:	out=print_object(item,depth);break;
    case cJSON_Binary:  out=print_binary(item,depth); break;
	}
	return out;
}
/* Render an object to text. */
static char *print_object(cJSON *item,int depth)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
	cJSON *child=item->child;
	int numentries=0,fail=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Explicitly handle empty object case */
	if (!numentries)
	{
		out=(char*)cJSON_malloc(depth * 3 +4);
		if (!out)	return 0;
		ptr=out;
		for (i=0;i<depth-1;i++){ *ptr++=' '; *ptr++=' '; };
		*ptr++=0;
		return out;
	}
	/* Allocate space for the names and the objects */
	entries=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	names=(char**)cJSON_malloc(numentries*sizeof(char*));
	if (!names) {cJSON_free(entries);return 0;}
	memset(entries,0,sizeof(char*)*numentries);
	memset(names,0,sizeof(char*)*numentries);

	/* Collect all the results into our arrays: */
	child=item->child;depth++; len+=depth;
	while (child)
	{
		names[i]=str=strdup(child->string);
		entries[i++]=ret=print_value(child,depth);
		if (str && ret) len+=strlen(ret)+strlen(str)+2+(depth*3); else fail=1;
		child=child->next;
	}
	
	/* Try to allocate the output string */
	if (!fail) out=(char*)cJSON_malloc(len);
	if (!out) fail=1;

	/* Handle failure */
	if (fail)
	{
		for (i=0;i<numentries;i++) {if (names[i]) cJSON_free(names[i]);if (entries[i]) cJSON_free(entries[i]);}
		cJSON_free(names);cJSON_free(entries);
		return 0;
	}
	
	/* Compose the output: */
	ptr=out; *ptr=0;
  if( depth > 1 ) *ptr++='\n';
	for (i=0;i<numentries;i++)
	{
	  for (j=0;j<depth-1;j++){ *ptr++=' '; *ptr++=' '; };
		strcpy(ptr,names[i]);ptr+=strlen(names[i]);
		*ptr++=':'; *ptr++=' ';
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		cJSON_free(names[i]);
    cJSON_free(entries[i]);
    if( i < numentries - 1 ) *ptr++='\n';
	}
	
	cJSON_free(names);cJSON_free(entries);
	*ptr++=0;
	return out;	
}

/* Render the number nicely from the given item into a string. */
static char *print_number(cJSON *item)
{
	char *str;
	double d=item->valuedouble;
	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		str=(char*)cJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
		if (str) sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)cJSON_malloc(64);	/* This is a nice tradeoff. */
		if (str)
		{
			if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)sprintf(str,"%.0f",d);
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			sprintf(str,"%e",d);
			else												sprintf(str,"%f",d);
		}
	}
	return str;
}

char  *cJSON_Print_YAML(cJSON *item){return print_value(item,0);};
