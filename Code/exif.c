/* BUG IN CODE:
 * 	Didn't work with IMGXYZ file
 */
#include "exif.h"

static int machine_order;
static int exif_order;

static uint32_t tiff_offset = 0;
static uint32_t exif_offset = 0;
static uint32_t gps_offset = 0;

/* Size in bytes of the Types */
static int size[] = {
	0,
	1,
	1,
	2,
	4,
	8,
	1,
	1,
	2,
	4,
	8,
	4,
	8
};

int main(int argc, char *argv[])
{
	if(argc<2 || ( strcmp(argv[1],"--help") == 0 ) ){
		printf("Usage :\n");
		printf("%s fileName [options] ...\n",argv[0]);
		exit(EXIT_SUCCESS);
	}

	machine_endian();

/* Opening file */

	FILE *fp;
	
	fp = fopen(argv[1],"r");
	if(fp == NULL)
		errExit("fopen");

/* Check image */

	if(!check_image(fp)){
		printf("File not a JPEG Image!\n");
		exit(EXIT_SUCCESS);	
	}

/* Check if exif */

	if(!check_img_format(fp)){
		printf("EXIF header not present!\n");
		exit(EXIT_SUCCESS);
	}
	
/* As we read total 4 bytes till here, we need to go back 2 bytes to input app1 header marker (ffe1) */

	fseek(fp,2,SEEK_SET);


/* Take EXIF header */

	struct app1_header app1_hdr;
	get_app1_header_data(fp, &app1_hdr);

/* Take tiff header */
	struct TiffHeader tiff_hdr;
	get_tiff_header(fp, &tiff_hdr);
	
	tiff_offset = tiff_hdr.offset_ifd0;

/* Input IFD0 */
	struct IFD ifd_zero;
	get_ifd(fp,&ifd_zero,tiff_offset);

/* Print IFD0 */
		printf("\n\n-----------------------------------------------------%s------------------------------------------\n\n","BASE_DATA");
	print_ifd(&ifd_zero);

/* Input EXIF IFD */
	if(exif_offset!=0){
		struct IFD exif_ifd;
		get_ifd(fp, &exif_ifd, exif_offset);

/* Print EXIF IFD */
		printf("\n\n-----------------------------------------------------%s------------------------------------------\n\n","EXIF_DATA");
		print_ifd(&exif_ifd);
	}

	if(gps_offset!=0){
/* Input GPS IFD */
		struct IFD gps_ifd;
		get_ifd(fp,&gps_ifd, gps_offset);

/* Print GPS IFD */
		printf("\n\n-----------------------------------------------------%s------------------------------------------\n\n","GPS_DATA");
		print_ifd(&gps_ifd);
	}

/* Closing file */

	fclose(fp);

	exit(EXIT_SUCCESS);
}

int check_image(FILE* fp)
{
	ssize_t data;
	unsigned char buf[2];
	unsigned char temp[] = {0xff,JPEG};

	data = fread(&buf,sizeof(buf),1,fp);
	if(data<=0)
		return FALSE;

	if(memcmp((void *)buf,(void *)temp,sizeof(buf)) == 0){
		return TRUE;
	}

	return FALSE;
}

int check_img_format(FILE* fp)
{
	ssize_t data;
	unsigned char buf[2];
	unsigned char exif[] = {0xff,EXIF};
	
	data = fread(&buf,sizeof(buf),1,fp);
	if(data<=0)
		return FALSE;

	if(memcmp((void *)buf,(void *)exif,sizeof(buf)) == 0)
		return TRUE;

	return FALSE;
}

void get_data(FILE* fp, void *buf, ssize_t size,size_t n)
{	
	ssize_t data;
	if((data = fread(buf,size,1,fp)) <= 0)
		errExit("fread");

}

void get_app1_header_data(FILE* fp, struct app1_header* hdr)
{
	unsigned char buf[sizeof(struct app1_header)];
	get_data(fp,&buf,sizeof(buf),1);

	memcpy((void *)hdr,(void *)buf,sizeof(buf));

}

void get_tiff_header(FILE* fp, struct TiffHeader *hdr)
{
	unsigned char buf[sizeof(struct TiffHeader)];
	unsigned char moto[] = {MOTOROLA,MOTOROLA};
	unsigned char intel[] = {INTEL,INTEL};
	get_data(fp,&buf,sizeof(buf),1);
	
// Determining EXIF byte order
	
	memcpy((void *)hdr,(void *)buf,sizeof(buf));
	if( memcmp( (void *)hdr->byte_order, (void *)moto, sizeof(moto) ) == 0)
		exif_order = BIG;
	else if( memcmp( (void *)hdr->byte_order, (void *)intel, sizeof(intel) ) == 0)
		exif_order = LIL;
	else
		errExit("get_tiff_header");

	

// Changing byte order to host order

	if(exif_order == BIG){
		hdr->offset_ifd0 = be32toh(hdr->offset_ifd0);
	}else{
		hdr->offset_ifd0 = le32toh(hdr->offset_ifd0);
	}

}

void get_ifd(FILE *fp, struct IFD *ifd, uint32_t offset)
{
	unsigned char buf[12];

// Seek fp to the offset where IFD is located
	fseek(fp,BASE_OFF+offset,SEEK_SET);

	get_data(fp,&(ifd->n),sizeof(ifd->n),1);

// Byte order change
	if(exif_order == BIG){
		ifd->n = be16toh(ifd->n);
	}else{
		ifd->n = le16toh(ifd->n);
	}


	ifd->ifd_dirent = (struct dirent *)malloc(sizeof(struct dirent)*(ifd->n));
	memset(ifd->ifd_dirent,0,(ifd->n)*sizeof(struct dirent));

// Using temporary file pointer to process individual direntries.
	FILE *tmpfp;
	tmpfp = fdopen( dup( fileno( fp ) ) , "r");


	for(uint16_t i = 0; i < ifd->n ; i++){
	
		get_data(fp,&buf,(sizeof(buf)),1);
		memcpy(&(ifd->ifd_dirent[i]),&buf,sizeof(buf));
	
		struct dirent* dir = &(ifd->ifd_dirent[i]);

		if(exif_order == BIG){
			dir->count = be32toh(dir->count);
			dir->offset = be32toh(dir->offset);
			dir->tag = be16toh(dir->tag);
			dir->type = be16toh(dir->type);		
		}else{
			dir->count = le32toh(dir->count);
			dir->offset = le32toh(dir->offset);
			dir->tag = le16toh(dir->tag);
			dir->type = le16toh(dir->type);		
		}

		get_ifd_data(tmpfp,dir);

	// Saving the EXIF Offset
		if(dir->tag == 34665){
			exif_offset = dir->offset;
		}


	// Saving the GPS Offset
		if(dir->tag == 34853){
			gps_offset = dir->offset;
		}

	}

	get_data(fp,&(ifd->offset),sizeof(ifd->offset),1);

	if(exif_order == BIG){
		ifd->offset = be32toh(ifd->offset);
	}else{
		ifd->offset = le32toh(ifd->offset);
	}

}

void get_ifd_data(FILE *fp, struct dirent *dir)
{
	uint32_t count = dir->count;
	uint32_t off = dir->offset;
	uint32_t flag = count*size[dir->type];
	fseek(fp,BASE_OFF+off,SEEK_SET);

/* If count > 4 then data located at offset */
	if(flag > 4){

		unsigned char *buf = (unsigned char *)malloc(flag);
		get_data(fp,buf,flag,1);

		dir->data.val = (unsigned char *)malloc(flag);
		memcpy(dir->data.val,buf,(flag));

		free(buf);

	}else{
	
		dir->data.val = (unsigned char *)malloc(sizeof(dir->offset));
		memcpy(dir->data.val,&(dir->offset),sizeof(dir->offset));

	}

}

void print_ifd(struct IFD *ifd)
{	
	struct dirent *dir = ifd->ifd_dirent;
	char *res;
	uint32_t count;

	for(int i=0; i < ifd->n ; i++){
		
		count = dir[i].count;
		res = find(dir+i);
		
		if(res == NULL){
			continue;
		}
		else{
			strcpy(dir[i].data.title,res);
		}

		printf("%-25s : ",dir[i].data.title);
		
		if(dir[i].type == 1 || dir[i].type == 6){
			for(int j = 0 ; j < count ; j++ )
				printf("%d",(int)dir[i].data.val[j]);
		}

		if(dir[i].type == 2 ){
			char c;

			for(int j=0 ; j < count; j++ ){

				c = dir[i].data.val[j];

				if(c == '\n')
					printf("\n");

				else if( c >=32 && c < 127 )
					printf("%c",c);
			}
		}


		if(dir[i].type == 3){
			for(int j = 0 ; j < count ; j++){

				if(exif_order == BIG)
					printf("%"PRIu16" ", ( (uint16_t *)dir[i].data.val)[count-j] );
				else
					printf("%"PRIu16" ", ( (uint16_t *)dir[i].data.val)[j] ) ;
			}
		}

		
		if(dir[i].type == 4){
			for(int j = 0 ; j < count ; j++ )
				printf("%"PRIu32" ",((uint32_t *)dir[i].data.val)[j]);
		}

		if(dir[i].type == 5){
			
			uint32_t numerator, denominator;
			for(int j = 0 ; j < 2*count ; j+=2 ){
				numerator = ( (uint32_t *)dir[i].data.val)[j];
				denominator =( (uint32_t *)dir[i].data.val)[j+1];
				
				if(exif_order == BIG){
					numerator = be32toh(numerator);
					denominator = be32toh(denominator);
				}
				if(denominator == 0)
					printf("%"PRIu32"/%"PRIu32"",numerator,denominator);
				else{
					float val = (float)numerator / (float)denominator;
					printf("%f ",val);
				}
			}

		}

		if(dir[i].type == 7){
			for(int j = 0 ; j < count ; j++ ){
				if(exif_order == BIG)
					printf("%d, ", ( (char *)dir[i].data.val)[count-j-1] );
				else
					printf("%d, ", ( (char *)dir[i].data.val)[j] ) ;
			}
		}

		if(dir[i].type == 8){
			for(int j = 0 ; j < count ; j++){

				if(exif_order == BIG)
					printf("%"PRId16" ", ( (int16_t *)dir[i].data.val)[count-j] );
				else
					printf("%"PRId16" ", ( (int16_t *)dir[i].data.val)[j] ) ;
			}
		}

		if(dir[i].type == 9){
			for(int j = 0 ; j < count ; j++ )
				printf("%d  SIGNED_LONG!!!!!!!!!",((int32_t *)dir[i].data.val)[j]);
		}
	
		if(dir[i].type == 10){
			int32_t numerator, denominator;
			for(int j = 0 ; j < 2*count ; j+=2 ){
				numerator = ( (int32_t *)dir[i].data.val)[j];
				denominator =( (int32_t *)dir[i].data.val)[j+1];

				if(exif_order == BIG){
					numerator = be32toh(numerator);
					denominator = be32toh(denominator);
				}

				float val = (float)numerator / (float)denominator;
				printf("%f SIGNED_RATIONAL!!!!!!",val);
			}
		}

		if(dir[i].type == 11){
			for(int j = 0 ; j < count ; j++ )
				printf("%f  FLOAT!!!!!!!!!",((float *)dir[i].data.val)[j]);
		}

		if(dir[i].type == 12){
			for(int j = 0 ; j < count ; j++ )
				printf("%lf  DOUBLE!!!!!!!!!",((double *)dir[i].data.val)[j]);
		}

		printf("\n");
	}
}



void errExit( char *str )
{
	printf("Error in :  %s\n",str);
	printf("%s\n",strerror(errno));
	exit(EXIT_FAILURE);
}

void machine_endian()
{
	unsigned int i = 1; 
	char *c = (char*)&i; 
	if (*c) 
		machine_order = LIL;
	else
		machine_order = BIG; 

	printf("Machine Order : %d\n",machine_order);
}
