//===================================================================================
//Read igd region data and query data, and then find all overlaps 
//by Jianglin Feng  05/12/2018
//database intervals sorted by _start: 8/12/2019
//-----------------------------------------------------------------------------------
#include "igd_create.h"

int create_help(int exit_code)
{
    fprintf(stderr,
"%s, v%s\n"
"usage:   %s create <input dir> <output dir> <output igd name> [options] \n"
"             -s  <Type of data structure> \n"
"                   0 for [index, start, end], default \n"
"                   1 for [index, start, end, value]\n"
"                   2 single combined BED file, datatype 1\n"
"             -c  < .BED column as value for data type 1 \n"
"                   >=4 (default) \n",
            PROGRAM_NAME, VERSION, PROGRAM_NAME);
    return exit_code;
}

//create ucsc igd from gz
void create_igd(char *iPath, char *oPath, char *igdName, int gtype)
{  	
	//0. Initialize igd
	igd_t *igd = igd_init();
	printf("igd_create 0\n");
	
    //1. Get the files  
    glob_t gResult;
    int rtn = glob(iPath, 0, NULL, &gResult);     
    if(rtn!=0){
        printf("wrong dir path: %s", iPath);
        return;
    }
    char** file_ids = gResult.gl_pathv;
    int32_t n_files = gResult.gl_pathc; 
    if(n_files<1)   
        printf("Too few files (add to path /*): %i\n", n_files);    
    int32_t *nr = calloc(n_files, sizeof(int32_t));
    double *avg = calloc(n_files, sizeof(double));
    printf("igd_create 1: %i\n", n_files); 
         
    //2. Read files
    int nCols=16;
    unsigned char buffer[256];    
    char **splits = malloc((nCols+1)*sizeof(char *));     
    int32_t i, j, k, ig, i0=0, i1=0, L0=0, L1=1, m, nL; //int64_t? 
    
    while(i0<n_files){
        //2.1 Start from (i0, L0): read till (i1, L1)
        ig = i0; 
        m = 0;              
		//2.2 Read ~4GB data from files
        while(m==0 && ig<n_files){   	//m>0 defines breaks when reading maxCount      
			//printf("%i, %i, %i, %s\n", i0, ig, nL, file_ids[ig]);
			gzFile fp;
			if ((fp = gzopen(file_ids[ig], "r")) == 0)
				return;                             
		    nL = 0; 
		    if(ig==i0 && L0>0){  		 //pass L0 lines of a big file
		        while(nL<L0 && gzgets(fp, buffer, 256)!=NULL)
		            nL++;              
		    }      			
			while (m==0 && gzgets(fp, buffer, 256)!=NULL) {
				str_splits(buffer, &nCols, splits); 
				int32_t  st = atol(splits[1]), en = atol(splits[2]), va = 0;
				if(nCols>4) va = atol(splits[4]);
				igd_add(igd, splits[0], st, en, va, ig);				
				nr[ig]++;
				avg[ig]+=en-st;
				nL++;
                if(igd->total>maxCount){
                    m = 1;
                    i1 = ig;
                    L1 = nL;    		//number of total lines or next line
                }
			}
			gzclose(fp);
			if(m==0) ig++;
		}
        //2.3 Save/append tiles to disc, add cnts tp Cnts           
		igd_saveT(igd, oPath);
        i0 = ig; 
        L0 = L1;
        L1 = 0;        
	}    
	printf("igd_create 2\n");
	
	//3. save _index.tsv: 4 columns--index, filename, nr, avg
    //Also has a header line: 
    char idFile[128];
    char *tchr;   
    sprintf(idFile, "%s%s%s", oPath, igdName, "_index.tsv");    
    FILE *fpi = fopen(idFile, "w");
    if(fpi==NULL)
        printf("Can't open file %s", idFile);     
    fprintf(fpi, "Index\tFile\tNumber of regions\tAvg size\n");    
    for(i=0; i<n_files; i++){
        tchr = strrchr(file_ids[i], '/');
        if(tchr!=NULL)
            tchr += 1;
        else
            tchr = file_ids[i];
        fprintf(fpi, "%i\t%s\t%i\t%f\n", i, tchr, nr[i], avg[i]/nr[i]);     
    }
    fclose(fpi);   
    free(nr);
    free(avg);  
    free(splits);  
    printf("igd_create 3\n");
    
	//4. Sort tile data and save into single files per ctg
	igd_save(igd, oPath, igdName);	
	globfree(&gResult); 
	printf("igd_create 4\n");
}

//create igd from a single .bed.gz Jaspar file with dataset index at 4th column
void create_igd_Jaspar(char *iPath, char *oPath, char *igdName)
{   //Process line by line: ipath is the file name
    //1. Get file_ids, n_files  
    char** file_ids;
    uint32_t n_files; 
    int nCols=16;
    unsigned char buffer[256]; 
    uint64_t nL;       
    char **splits = malloc((nCols+1)*sizeof(char *));     
    //-------------------------------------------------------------------------
    printf("igd_create 1\n");
    int i, maxF = 10000;
    file_ids = malloc(maxF*sizeof(char *));
    gzFile zfile;
    zfile = gzopen (iPath, "r");    
    if (!zfile) {
        fprintf (stderr, "gzopen of '%s' failed: %s.\n", iPath,
                 strerror (errno));
        exit (EXIT_FAILURE);
    } 
    n_files = 0;
    nL = 0;
    while(nL<200000000 && gzgets(zfile, buffer, 256)!=NULL){                   
        str_splits(buffer, &nCols, splits);      
        i=0;
        while(i<n_files && strcmp(splits[3], file_ids[i])!=0)
            i++;
        if(i==n_files){
            file_ids[i] = malloc(strlen(splits[3])+1);
            strcpy(file_ids[i], splits[3]);//splits tmp pointers  
            printf("%s %u %lld\n", file_ids[i], n_files, (long long)nL);
            n_files++;            
        }       
        nL++;
        if(nL%10000000==0)
            printf("\t%u %lld\n", n_files, (long long)nL);                
    }
    gzclose(zfile);    
    printf("%u %lld\n", n_files, (long long)nL);        
    
    //-----convert tf name to idx
	//void *hf = kh_init(str); 
	//int absent;
	//for(i=0;i<n_files;i++){ 
	//	khint_t k;
	//	strhash_t *h = (strhash_t*)hf;
	//	k = kh_put(str, h, file_ids[i], &absent);							
	//	kh_val(h, k) = i;			
	//	kh_key(h, k) = files_ids[i];
	//}     
    	
	//------------------------------------------------------------------------		
    igd_t *igd = igd_init(); 
    igd->gType = 1;  
    int32_t *nr = calloc(n_files, sizeof(int32_t));
    double *avg = calloc(n_files, sizeof(double));
         
    //2. Read file  
    nL = 0;
    int j=0;                          
    zfile = gzopen(iPath, "r");              
    while(gzgets(zfile, buffer, 256)!=NULL){ 
        str_splits(buffer, &nCols, splits);	      
		//strhash_t *h = (strhash_t*)hf;
		//khint_t k = kh_get(str, h, splits[3]);
		//k = kh_val(h, k);        
        i=0;
        while(i<n_files && strcmp(splits[3], file_ids[i])!=0)
            i++;   
        int32_t st = atol(splits[1]), en = atol(splits[2]); 	
		igd_add(igd, splits[0], st, en, atol(splits[4]), i);			
		nr[i]++;
		avg[i]+=en-st;
		nL++;	
        //Save/append tiles to disc, add cnts tp Cnts 
        if(igd->total>=maxCount){ 
        	j++;
        	printf("--igd_saveT1--%i, %lld\n", j, (long long)igd->total);
			igd_saveT(igd, oPath);
		    nL = 0;        
        }
	}	
	gzclose(zfile); 
	if(nL>0)
		igd_saveT(igd, oPath);  
	printf("igd_create 2\n");
	
	//3. save _index.tsv: 4 columns--index, filename, nr, avg
    //Also has a header line: 
    char idFile[128];
    char *tchr;   
    sprintf(idFile, "%s%s%s", oPath, igdName, "_index.tsv");    
    FILE *fpi = fopen(idFile, "w");
    if(fpi==NULL)
        printf("Can't open file %s", idFile);     
    fprintf(fpi, "Index\tFile\tNumber of regions\tAvg size\n");    
    for(i=0; i<n_files; i++){
        tchr = strrchr(file_ids[i], '/');
        if(tchr!=NULL)
            tchr += 1;
        else
            tchr = file_ids[i];
        fprintf(fpi, "%i\t%s\t%i\t%f\n", i, tchr, nr[i], avg[i]/nr[i]);     
    }
    fclose(fpi);   
    free(nr);
    free(avg); 
    free(splits);     
    //kh_destroy(str, (strhash_t*)hf);  
    printf("igd_create 3\n");
	//4. Sort tile data and save into a single file
	igd_save(igd, oPath, igdName);	
	printf("igd_create 4\n");     
}

//-------------------------------------------------------------------------------------
int igd_create(int argc, char **argv)
{
    if (argc < 5) 
        return create_help(EX_OK);       

    int32_t i, j, n;
    char ipath[128];
    char opath[128];
    char *s1, *s2;
    strcpy(ipath, argv[2]);
    strcpy(opath, argv[3]);
    char *dbname = argv[4]; 
    int dtype = 0;
    
    for(i=5; i<argc; i++){
        if(strcmp(argv[i], "-s")==0 && i+1<argc)//data structure type
            dtype = atoi(argv[i+1]);                        
    }
    if(opath[strlen(opath)-1]!='/'){
        strcat(opath, "/");
    }          
      
    if(dtype!=2){                            
        if(ipath[strlen(ipath)-1]=='/'){
            strcat(ipath, "*");
        }
        else if(ipath[strlen(ipath)-1]!='*'){
            strcat(ipath, "/*");
        }
    }  
    
    //check if the subfolders exist:    
    char ftmp[128];      
    struct stat st = {0};  
    
    sprintf(ftmp, "%s%s%s", opath, dbname, ".igd");
    if(stat(ftmp, &st) == 0)
        printf("The igd database file %s exists!\n", ftmp);  
    else{
        if (stat(opath, &st) == -1){
            mkdir(opath, 0777);    
        }
        sprintf(ftmp, "%s%s", opath, "data0");
        if (stat(ftmp, &st) == -1)
            mkdir(ftmp, 0777);
        if(dtype!=2)
        	create_igd(ipath, opath, dbname, dtype);  
        else
         	create_igd_Jaspar(ipath, opath, dbname);             
    } 

    return EX_OK;
}

