//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2024/2025
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h> // Consider <cstdlib> in C++
#include <math.h>
#include <opencv2/core/types_c.h>     // For IplImage and basic types
#include <opencv2/imgproc/imgproc_c.h> // For processing functions if needed

#include "vc.h" // Include your own header

// extern "C" starts HERE, wrapping only function definitions
#ifdef __cplusplus
extern "C" {
#endif
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar mem�ria para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar mem�ria de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;

	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}

	t = tok;

	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));

		if(c == '#') ungetc(c, file);
	}

	*t = 0;

	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;

	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}

		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 ||
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca mem�ria para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}

		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}

	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;

	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);

			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);

			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}

		fclose(file);

		return 1;
	}

	return 0;
}


int vc_gray_negative(IVC *srcdst) {
	unsigned char *data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width*srcdst->channels;
	int channel = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))return 0;
	if (channel != 1)return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y*bytesperline + x*channel;

			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}

int vc_rgb_negative(IVC *srcdst) {
	unsigned char *data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width*srcdst->channels;
	int channel = srcdst->channels;
	int x, y;
	long int pos;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL))return 0;
	if (channel != 3)return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y*bytesperline + x*channel;

			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 3)||(dst->channels!=1))return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src+1];
			bf= (float)datasrc[pos_src+2];

			datadst[pos_dst] = (unsigned char)((rf*0.229) + (gf*0.587) + (bf*0.114));
		}
	}
	return 1;
}



int vc_rgb_to_hsv(IVC *srcdst, int tipo)
{
	unsigned char *data = (unsigned char *)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	float r, g, b, hue, saturation, value;
	float rgb_max, rgb_min;
	int i, size;

	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	size = width * height * channels;

	for (i = 0; i<size; i = i + channels)
	{
		r = (float)data[i];
		g = (float)data[i + 1];
		b = (float)data[i + 2];

		// Calcula valores maximo e minimo dos canais de cor R, G e B
		rgb_max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
		rgb_min = (r < g ? (r < b ? r : b) : (g < b ? g : b));

		// Value toma valores entre [0,255]
		value = rgb_max;
		if (value == 0.0)
		{
			hue = 0.0;
			saturation = 0.0;
		}
		else
		{
			// Saturation toma valores entre [0,255]
			saturation = ((rgb_max - rgb_min) / rgb_max) * 255.0f;

			if (saturation == 0.0)
			{
				hue = 0.0f;
			}
			else
			{
				// Hue toma valores entre [0,360]
				if ((rgb_max == r) && (g >= b))
				{
					hue = 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if ((rgb_max == r) && (b > g))
				{
					hue = 360.0f + 60.0f * (g - b) / (rgb_max - rgb_min);
				}
				else if (rgb_max == g)
				{
					hue = 120.0f + 60.0f * (b - r) / (rgb_max - rgb_min);
				}
				else /* rgb_max == b*/
				{
					hue = 240.0f + 60.0f * (r - g) / (rgb_max - rgb_min);
				}
			}
		}


		if (tipo == 0) { // moedas 10 20 50 e 1 euro para binario
			if ((hue >= 40.0f) && (hue <= 85.0f) && saturation >= 50.0f&&value >= 50.0f) {
				data[i] = 255;
				data[i + 1] = 255;
				data[i + 2] = 255;
			}
			else {
				data[i] = 0;
				data[i + 1] = 0;
				data[i + 2] = 0;
			}
		}
		else if (tipo == 1) { //moedas 1 2 5 centimos para binario
			if ((hue >= 15.0f) && (hue <= 40.0f) && (saturation >= 80.0f)) {
				data[i] = 255;
				data[i + 1] = 255;
				data[i + 2] = 255;
			}
			else {
				data[i] = 0;
				data[i + 1] = 0;
				data[i + 2] = 0;
			}
		}

	}


	return 1;
}


int vc_scale_gray_to_rgb(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 3))return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;


			if (datasrc[pos_src] <= 64) {
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = datasrc[pos_src]*(255/64);
				datadst[pos_dst + 2] = 255;
			}
			else if (datasrc[pos_src] <= 128) {
				datadst[pos_dst] = 0;
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 255 - (datasrc[pos_src] - 64)*(255 / 64);
			}
			else if (datasrc[pos_src] <= 192) {
				datadst[pos_dst] = (datasrc[pos_src]-128) * (255 / 64);
				datadst[pos_dst + 1] = 255;
				datadst[pos_dst + 2] = 0;
			}
			else {
				datadst[pos_dst] = 255;
				datadst[pos_dst + 1] = 255 - (datasrc[pos_src] - 192)*(255 / 64);
				datadst[pos_dst + 2] = 0;
			}

		}
	}
	return 1;
}


int vc_gray_to_binary(IVC *src, IVC *dst, int threshold) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}

		}
	}

	return 1;
}


int vc_gray_to_binary_global_mean(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;
	float threshold=0;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;

	for ( y = 0; y < height; y++)
	{
		for  (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			threshold += datasrc[pos_src];
		}
	}

	threshold = threshold / (width*height);

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y*bytesperline_src + x*channels_src;
			pos_dst = y*bytesperline_dst + x*channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}

		}
	}
	return 1;
}

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	float threshold = 0;
	int max, min;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;
	kernel *= 0.5;
	for (y = kernel; y < height-kernel; y++)
	{
		for (x = kernel; x < width-kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			for (max=0,min=255,y2 = y-kernel; y2 <= y+kernel; y2++)
			{
				for (x2 = x-kernel; x2 <= x+kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] > max) { max = datasrc[pos_src]; }
					else if (datadst[pos_src] < min) { min = datasrc[pos_src]; }
				}
			}
			threshold = 0.5*(max + min);

			pos_src = y*bytesperline_src + x*channels_src;
			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else
			{
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_binary_dilate(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height-kernel; y++)
	{
		for (x = kernel; x < width-kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			verifica = 0;

			for (y2 = y-kernel; y2 <= y+kernel; y2++)
			{
				for (x2 = x-kernel; x2 <= x+kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] == 255) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 255; }
			else { datadst[pos_dst] = 0; }
		}
	}

	return 1;
}

int vc_binary_erode(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int verifica;
	kernel *= 0.5;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;

			verifica = 0;

			for (y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					if (datasrc[pos_src] == 0) { verifica = 1; }
				}
			}

			if (verifica == 1) { datadst[pos_dst] = 0; }
			else { datadst[pos_dst] = 255; }

		}
	}


	return 1;
}


int vc_binary_open(IVC *src, IVC *dst, int kernel) {
	int verifica=1;
	IVC *dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_erode(src, dstTemp, kernel);
	verifica &= vc_binary_dilate(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}


int vc_binary_close(IVC *src, IVC *dst, int kernel) {
	int verifica = 1;
	IVC *dstTemp = vc_image_new(src->width, src->height, src->channels, src->levels);

	verifica &= vc_binary_dilate(src, dstTemp, kernel);
	verifica &= vc_binary_erode(dstTemp, dst, kernel);

	vc_image_free(dstTemp);

	return verifica;
}




OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels)
{
	unsigned char *datasrc = (unsigned char *)src->data;
	unsigned char *datadst = (unsigned char *)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC *blobs; // Apontador para array de blobs (objectos) que ser� retornado desta fun��o.

				// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem bin�ria para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pix�is de plano de fundo devem obrigat�riamente ter valor 0
	// Todos os pix�is de primeiro plano devem obrigat�riamente ter valor 255
	// Ser�o atribu�das etiquetas no intervalo [1,254]
	// Este algoritmo est� assim limitado a 255 labels
	for (i = 0, size = bytesperline * height; i<size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem bin�ria
	for (y = 0; y<height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x<width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

													// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A est� marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D est� marcado, e � menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a<label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y<height - 1; y++)
	{
		for (x = 1; x<width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do n�mero de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a<label - 1; a++)
	{
		for (b = a + 1; b<label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que n�o hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a<label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se n�o h� blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC *)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a<(*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;



	return blobs;
}


int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs)
{
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verifica��o de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta �rea de cada blob
	for (i = 0; i<nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y<height - 1; y++)
		{
			for (x = 1; x<width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// �rea
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Per�metro
					// Se pelo menos um dos quatro vizinhos n�o pertence ao mesmo label, ent�o � um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x,y;
	float min;
	long int pos_src, pos_dst;
	int posicoes[256] = {0};
	float pdf[256] = { 0 }, cdf[256] = { 0 };
	float size = width*height;

	for (x = 0; x < height*width; x++)
	{
		posicoes[datasrc[x]]++;
	}
	for (x = 0; x <= 255; x++)
	{
		pdf[x] = (((float)posicoes[x]) / size);
	}

	for (cdf[0] = pdf[0] ,x = 1; x <= 255; x++)
	{
		cdf[x] = cdf[x - 1] + pdf[x];
	}


	for (min = 0, x = 0; x <= 255; x++)
	{
		if (cdf[x] != 0) { min = pdf[x]; break; }
	}

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_dst = y*bytesperline_src + x*channels_src;
			datadst[pos_dst] = ((cdf[datasrc[pos_dst]] - min) / (1.0 - min))*255.0;
		}
	}

	return 1;
}






int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th) {
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width*src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((width <= 0) || (height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1)* byteperline + (x - 1) * channels;
			posB = (y - 1)* byteperline + (x)* channels;
			posC = (y - 1)* byteperline + (x + 1)* channels;
			posD = (y)* byteperline + (x - 1)* channels;
			posE = (y)* byteperline + (x + 1)* channels;
			posF = (y + 1)* byteperline + (x - 1)* channels;
			posG = (y + 1)* byteperline + (x)* channels;
			posH = (y + 1)* byteperline + (x + 1)* channels;
			mx = (double)((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3.0;
			my = (double)((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3.0;

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_edge_sobel(IVC *src, IVC *dst, float th) {
	unsigned char *data = (unsigned char *)src->data;
	int width = src->width;
	int height = src->height;
	int byteperline = src->width*src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	long int posA, posB, posC, posD, posE, posF, posG, posH;
	double mag, mx, my;

	if ((width <= 0) || (height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 1; y < height; y++)
	{
		for (x = 1; x < width; x++)
		{
			pos = y * byteperline + x * channels;

			posA = (y - 1)* byteperline + (x - 1) * channels;
			posB = (y - 1)* byteperline + (x)* channels;
			posC = (y - 1)* byteperline + (x + 1)* channels;
			posD = (y)* byteperline + (x - 1)* channels;
			posE = (y)* byteperline + (x + 1)* channels;
			posF = (y + 1)* byteperline + (x - 1)* channels;
			posG = (y + 1)* byteperline + (x)* channels;
			posH = (y + 1)* byteperline + (x + 1)* channels;

			mx = (double)((-1 * data[posA]) + (1 * data[posC]) + (-2 * data[posD]) + (2 * data[posE]) + (-1 * data[posF]) + (1 * data[posH])) / 3;
			my = (double)((-1 * data[posA]) + (1 * data[posF]) + (-2 * data[posB]) + (2 * data[posG]) + (-1 * data[posC]) + (1 * data[posH])) / 3;

			mag = sqrt((mx*mx) + (my * my));

			if (mag > th)
				dst->data[pos] = 255;
			else
				dst->data[pos] = 0;
		}
	}
	return 1;
}

int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int media, mediaDiv=kernel^2;
	kernel *= 0.5;


	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	//if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;


			for (media = 0, y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					media += datasrc[pos_src];
				}
			}
			media /= mediaDiv;
			datadst[pos_dst] = media;
		}
	}


	return 1;
}

int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernel) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y, x2, y2;
	long int pos_src, pos_dst;
	int mediana, mediaDiv = kernel ^ 2;
	kernel *= 0.5;
	int i, j,k, temp;
	int *a = (int *)malloc(sizeof(int)*mediaDiv);

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	//if ((src->channels != 1) || (dst->channels != 1))return 0;


	for (y = kernel; y < height - kernel; y++)
	{
		for (x = kernel; x < width - kernel; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;


			for (k=0,y2 = y - kernel; y2 <= y + kernel; y2++)
			{
				for (x2 = x - kernel; x2 <= x + kernel; x2++)
				{
					pos_src = y2*bytesperline_src + x2*channels_src;
					a[k] = datasrc[pos_src];
				}
			}



			for (i = 0; i < (mediaDiv - 1); ++i)
			{
				for (j = i + 1; j < mediaDiv; ++j)
				{
					if (a[i] > a[j])
					{
						temp = a[j];
						a[j] = a[i];
						a[i] = temp;
					}
				}
			}

				mediana = (mediaDiv / 2) + 1;

				datadst[pos_dst] = a[mediana];
		}
	}


	return 1;


}

int vc_gray_highpass_filter(IVC *src, IVC *dst) {
	unsigned char *datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width*src->channels;
	int channels_src = src->channels;
	unsigned char *datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_dst = dst->width*dst->channels;
	int channels_dst = dst->channels;
	int x, y;
	int soma;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL))return 0;
	if ((src->width != dst->width) || (src->height != dst->height))return 0;
	if ((src->channels != 1) || (dst->channels != 1))return 0;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			pos_dst = y*bytesperline_dst + x*channels_dst;


			soma = datasrc[(y - 1)* bytesperline_src + (x - 1) * channels_src] * (-1);
			soma += datasrc[(y - 1)* bytesperline_src + (x)* channels_src] * (-2);
			soma += datasrc[(y - 1)* bytesperline_src + (x + 1)* channels_src] * (-1);
			soma += datasrc[(y)* bytesperline_src + (x - 1)* channels_src] * (-2);
			soma += datasrc[(y)* bytesperline_src + (x + 1)* channels_src] * (-2);
			soma += datasrc[(y + 1)* bytesperline_src + (x - 1)* channels_src] * (-1);
			soma += datasrc[(y + 1)* bytesperline_src + (x)* channels_src] * (-2);
			soma += datasrc[(y + 1)* bytesperline_src + (x + 1)* channels_src] * (-1);
			soma += datasrc[pos_dst] * (12);

			soma = (unsigned char)MIN(MAX((float)datasrc[pos_dst]+((float)soma / (float)16) * 6.0, 0), 255);
			//datadst[pos_dst] = (unsigned char)((float)abs(soma) / (float)16);

			datadst[pos_dst] = soma;

		}
	}

	return 1;
}



int vc_bgr_to_rgb(IplImage *imagemEntrada, IVC *imagemSaida)
{
	int x, y, pos;

	if ((imagemSaida->width <= 0) || (imagemSaida->height <= 0)) {
		return 0;
	}

	if (imagemSaida->channels != 3) {
		return 0;
	}

	for (y = 0; y < imagemSaida->height; y++)
	{
		for (x = 0; x < imagemSaida->width; x++)
		{
			pos = y*imagemSaida->bytesperline + x*imagemSaida->channels;

			imagemSaida->data[pos] = imagemEntrada->imageData[pos + 2];
			imagemSaida->data[pos + 1] = imagemEntrada->imageData[pos + 1];
			imagemSaida->data[pos + 2] = imagemEntrada->imageData[pos];
		}
	}

	return 0;
}

int vc_desenha_box(IplImage *src, OVC *blobs, int nBlobs) {
	unsigned char* datasrc = (unsigned char *)src->imageData;
	int bytesperline_src = src->width*src->nChannels;
	int channels_src = src->nChannels;
	int i, xx, yy, pos = 0, posCentroMassa;

	for (i = 0; i < nBlobs; i++)
	{
		if (blobs[i].area>9000 && blobs[i].height <= 180) {
			//centro de massa
			posCentroMassa = blobs[i].yc * bytesperline_src + blobs[i].xc * channels_src;
			datasrc[posCentroMassa] = 252;
			datasrc[posCentroMassa + 1] = 2;
			datasrc[posCentroMassa + 2] = 139;


			for (yy = blobs[i].y; yy <= blobs[i].y + blobs[i].height; yy++)
			{
				for (xx = blobs[i].x; xx <= blobs[i].x + blobs[i].width; xx++)
				{
					pos = yy * bytesperline_src + xx * channels_src;

					if ((yy == blobs[i].y || yy == blobs[i].y + blobs[i].height || xx == blobs[i].x || xx == blobs[i].x + blobs[i].width))
					{
						datasrc[pos] = 252;
						datasrc[pos + 1] = 2;
						datasrc[pos + 2] = 139;

					}
				}
			}
		}
	}
	return 1;
}

int exclui(int *exclui, int xc, int opcao) {
	int i;
	if (opcao == 0) {
		for (i = 0; i < 10; i++)
		{
			if (exclui[i] == 0) {
				exclui[i] = xc;
				break;
			}
		}
	}
	else if (opcao == 1) {
		for (i = 0; i < 10; i++)
		{
			if (exclui[i] <= xc + 10 && exclui[i] >= xc - 10) {
				exclui[i] = 0;
				break;
			}
		}
	}
	else return 0;
	return 0;
}



int tipo(int diametro, int tipo, int *total) {
	if (diametro >= 155 && diametro < 170) {
		if (tipo == 0) {
			{ total[4]++; return 20; }
		}
		else if (tipo == 1) {
			{ total[2]++; return 5; }
		}
	}
	//20

	if (diametro < 155 && diametro>136) {
		if (tipo == 0) {
			{ total[3]++; return 10; } //10 ou 5
		}
		else if (tipo == 1) {
			{ total[2]++; return 5; }
		}
	} //10
	if (diametro >= 170) { total[5]++; return 50; } //50
	if (diametro <= 135 && diametro >= 129) { total[1]++; return 2; } //2
	if (diametro < 129) { total[0]++; return 1; } //1
	else return -1;
}

void ProcessaImagem(OVC *blobs, OVC *blobs2, OVC *blobs3, int nlabels, int nlabels2, int nlabels3, int *excluir, int *total) {
	int i, i2, i3, i4, ff = 0; //ff verifica se est� na lista para excluir


	for (i = 0; i < nlabels; i++)
	{

		if (blobs[i].area>10000 && blobs[i].area<25000 && blobs[i].yc<650 && blobs[i].yc>550 && blobs[i].width <= 200) {

			for (i3 = 0; i3 < 10; i3++)
			{
				if (excluir[i3] <= (blobs[i].xc + 15) && excluir[i3] >= (blobs[i].xc - 15)) {
					ff = 1;
					break;
				}
				else { ff = 0; }
			}
			if (ff == 0) {
				for (i2 = 0; i2 < nlabels2; i2++)
				{
					if (blobs2[i2].area>10500 && blobs2[i2].yc <= (blobs[i].yc + 30) && blobs2[i2].yc >= (blobs[i].yc - 30) && blobs2[i2].xc <= (blobs[i].xc + 30) && blobs2[i2].xc >= (blobs[i].xc - 30)) {
						int temp = tipo(blobs2[i2].width, 0, total);
						printf("*MOEDA* Valor:%d centimos | Area:%d | Perimetro %d\n", temp, blobs2[i2].area, blobs2[i2].perimeter);
						exclui(excluir, blobs2[i2].xc, 0);
						break;
					}
					else if (blobs2[i2].area < 10500 && blobs2[i2].area>7000 && blobs2[i2].yc <= (blobs[i].yc + 30) && blobs2[i2].yc >= (blobs[i].yc - 30) && blobs2[i2].xc <= (blobs[i].xc + 30) && blobs2[i2].xc >= (blobs[i].xc - 30)) {
						printf("*MOEDA* Valor:1 euro | Area:%d | Perimetro %d\n", blobs2[i2].area, blobs2[i2].perimeter);
						exclui(excluir, blobs2[i2].xc, 0);
						total[6]++;
						break;
					}
				}
				for (i4 = 0; i4 < nlabels3; i4++)
				{
					if (blobs3[i4].area>10000 && blobs3[i4].yc <= (blobs[i].yc + 30) && blobs3[i4].yc >= (blobs[i].yc - 30) && blobs3[i4].xc <= (blobs[i].xc + 30) && blobs3[i4].xc >= (blobs[i].xc - 30)) {
						int temp = tipo(blobs3[i4].width, 1, total);
						printf("*MOEDA* Valor:%d centimo/s | Area:%d | Perimetro %d\n", temp, blobs3[i4].area, blobs3[i4].perimeter);
						exclui(excluir, blobs3[i4].xc, 0);
						break;
					}
				}
			}
		}
		else if (blobs[i].yc >= 400 && blobs[i].yc <= 550) {
			exclui(excluir, blobs[i].xc, 1);
		}
	}
}


void ProcessaFrame(IplImage *frame, IplImage *frame2, int *excluir, int *total) {
	IVC *imagem, *binaria, *hsv, *gray, *binaria2, *gray2, *binaria3, *gray3, *hsv2;
	OVC *blobs, *blobs2, *blobs3;
	int nlabels, nlabels2, nlabels3;

	imagem = vc_image_new(frame->width, frame->height, frame->nChannels, 255);
	gray = vc_image_new(frame->width, frame->height, 1, 255);
	gray2 = vc_image_new(frame->width, frame->height, 1, 255);
	gray3 = vc_image_new(frame->width, frame->height, 1, 255);
	binaria = vc_image_new(frame->width, frame->height, 1, 255);
	binaria2 = vc_image_new(frame->width, frame->height, 1, 255);
	binaria3 = vc_image_new(frame->width, frame->height, 1, 255);
	hsv = vc_image_new(frame->width, frame->height, 3, 255);
	hsv2 = vc_image_new(frame->width, frame->height, 3, 255);


	vc_bgr_to_rgb(frame, imagem);
	hsv->data = imagem->data;
	//original binaria
	vc_rgb_to_gray(imagem, gray);
	vc_gray_negative(gray);
	vc_gray_to_binary(gray, binaria, 150);
	//hsv1
	vc_rgb_to_hsv(hsv, 0); //passa automaticamente para "binario" no final da fun��o
	vc_rgb_to_gray(hsv, gray2);
	vc_gray_to_binary(gray2, binaria2, 100);
	vc_binary_open(binaria2, gray2, 5);
	//hsv2
	vc_bgr_to_rgb(frame2, imagem);
	hsv2->data = imagem->data;
	vc_rgb_to_hsv(hsv2, 1); //passa automaticamente para "binario" no final da fun��o
	vc_rgb_to_gray(hsv2, gray3);
	vc_gray_to_binary(gray3, binaria3, 100);
	vc_binary_open(binaria3, gray3, 5);


	blobs = vc_binary_blob_labelling(binaria, binaria, &nlabels);
	vc_binary_blob_info(binaria, blobs, nlabels);
	blobs2 = vc_binary_blob_labelling(gray2, gray2, &nlabels2);
	vc_binary_blob_info(gray2, blobs2, nlabels2);
	blobs3 = vc_binary_blob_labelling(gray3, gray3, &nlabels3);
	vc_binary_blob_info(gray3, blobs3, nlabels3);

	ProcessaImagem(blobs, blobs2, blobs3, nlabels, nlabels2, nlabels3, excluir, total);

	vc_desenha_box(frame, blobs2, nlabels2);
	vc_desenha_box(frame, blobs3, nlabels3);


	free(blobs);
	free(blobs2);
	free(blobs3);
}

#ifdef __cplusplus
}
#endif