#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include "pt2_header.h"
#include "pt2_structs.h"
#include "ppt_pattern_names.h"
#define NAMES_MAGIC "# Pocket Pro Tracker pattern names v1"
static char pendingPath[PATH_MAX+1];
static bool sidecarPath(char *out,size_t capacity,const char *modulePath){
    return snprintf(out,capacity,"%s.patterns.txt",modulePath)<(int)capacity;
}
bool ppt_pattern_names_save(const char *modulePath){
    char target[PATH_MAX+64],temp[PATH_MAX+80];
    if(!sidecarPath(target,sizeof(target),modulePath))return false;
    if(snprintf(temp,sizeof(temp),"%s.tmp.XXXXXX",target)>=(int)sizeof(temp))return false;
    int fd=mkstemp(temp);if(fd<0)return false;
    FILE *f=fdopen(fd,"w");if(!f){close(fd);unlink(temp);return false;}
    bool ok=fprintf(f,"%s\n",NAMES_MAGIC)>0;
    for(int i=0;i<MAX_PATTERNS && ok;i++){
        const char *name=ppt_pattern_name(i);if(!name || !name[0])continue;
        char clean[16]={0};
        for(int j=0;j<15 && name[j];j++){
            unsigned char c=(unsigned char)name[j];clean[j]=c>=32 && c<=126?(char)c:' ';
        }
        ok=fprintf(f,"%02d\t%s\n",i,clean)>0;
    }
    if(fclose(f)!=0)ok=false;
    if(ok && rename(temp,target)==0)return true;
    unlink(temp);return false;
}
void ppt_pattern_names_prepare(const char *modulePath){
    pendingPath[0]=0;
    if(modulePath[0]=='/'){
        if(strlen(modulePath)<sizeof(pendingPath))strcpy(pendingPath,modulePath);
    }else{
        char cwd[PATH_MAX+1];if(!getcwd(cwd,sizeof(cwd)))return;
        if(snprintf(pendingPath,sizeof(pendingPath),"%s/%s",cwd,modulePath)>=(int)sizeof(pendingPath))pendingPath[0]=0;
    }
}
bool ppt_pattern_names_load(void){
    char path[PATH_MAX+64];
    if(!pendingPath[0])return true;
    bool validPath=sidecarPath(path,sizeof(path),pendingPath);pendingPath[0]=0;
    if(!validPath)return false;
    FILE *f=fopen(path,"r");if(!f)return errno==ENOENT;
    char names[MAX_PATTERNS][16];memset(names,0,sizeof(names));
    bool seen[MAX_PATTERNS]={false},ok=true;char line[256];int lines=0;
    if(!fgets(line,sizeof(line),f))ok=false;
    if(ok){line[strcspn(line,"\r\n")]=0;if(strcmp(line,NAMES_MAGIC))ok=false;}
    while(ok && fgets(line,sizeof(line),f)){
        if(++lines>1024){ok=false;break;}
        size_t n=strcspn(line,"\r\n");
        if(n==sizeof(line)-1){ok=false;break;}line[n]=0;
        if(!n || line[0]=='#')continue;
        if(n<3 || line[0]<'0' || line[0]>'9' || line[1]<'0' || line[1]>'9' || line[2]!='\t' || n-3>15){ok=false;break;}
        int index=(line[0]-'0')*10+line[1]-'0';
        if(index>=MAX_PATTERNS || seen[index]){ok=false;break;}
        seen[index]=true;
        for(size_t i=3;i<n;i++)if((unsigned char)line[i]<32 || (unsigned char)line[i]>126){ok=false;break;}
        if(ok)memcpy(names[index],line+3,n-3);
    }
    if(ferror(f))ok=false;
    fclose(f);if(!ok)return false;
    for(int i=0;i<MAX_PATTERNS;i++)memcpy(ppt_pattern_name(i),names[i],16);
    ui.updatePosEd=true;return true;
}
