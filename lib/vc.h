//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLIT�CNICO DO C�VADO E DO AVE
//                          2011/2012
//             ENGENHARIA DE SISTEMAS INFORM�TICOS
//                    VIS�O POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#ifdef __cplusplus
extern "C" {
#endif

//#include <opencv/cv.h>
#include <opencv2/core/types_c.h>     // For IplImage and basic types

#define VC_DEBUG

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//							 MACROS
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Bin�rio/Cinzentos=1; RGB=3
	int levels;				// Bin�rio=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// �rea
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Per�metro
	int label;					// Etiqueta
} OVC;



//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROT�TIPOS DE FUN��ES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUN��ES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUN��ES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

//
int vc_gray_negative(IVC *srcdst);
int vc_rgb_negative(IVC *srcdst);

int vc_rgb_to_gray(IVC *src, IVC *dst);

int vc_rgb_to_hsv(IVC *srcdst, int tipo);

int vc_scale_gray_to_rgb(IVC *src ,IVC *dst);

int vc_gray_to_binary(IVC *src, IVC *dst, int threshold);

int vc_gray_to_binary_global_mean(IVC *src, IVC *dst);

int vc_gray_to_binary_midpoint(IVC *src, IVC *dst, int kernel);

int vc_binary_dilate(IVC *src, IVC *dst, int kernel);

int vc_binary_erode(IVC *src, IVC *dst, int kernel);

int vc_binary_open(IVC *src, IVC *dst, int kernel);

int vc_binary_close(IVC *src, IVC *dst, int kernel);

//int vc_binary_blob_labelling(IVC *src, IVC *dst);

OVC* vc_binary_blob_labelling(IVC *src, IVC *dst, int *nlabels);

int vc_binary_blob_info(IVC *src, OVC *blobs, int nblobs);

int vc_gray_edge_prewitt(IVC *src, IVC *dst, float th);

int vc_gray_edge_sobel(IVC *src, IVC *dst, float th);


int vc_gray_lowpass_mean_filter(IVC *src, IVC *dst, int kernel);
int vc_gray_lowpass_median_filter(IVC *src, IVC *dst, int kernel);
int vc_gray_lowpass_gaussian_filter(IVC *src, IVC *dst);
int vc_gray_highpass_filter(IVC *src, IVC *dst);




//int vc_gray_histogram_show(IVC *src, IVC *dst);

int vc_gray_histogram_equalization(IVC *src, IVC *dst);

//trab


int vc_bgr_to_rgb(IplImage *imagemEntrada, IVC *imagemSaida);
void ProcessaFrame(IplImage *frame, IplImage *frame2, int *excluir, int *total);
void ProcessaImagem(OVC *blobs, OVC *blobs2, OVC *blobs3, int nlabels, int nlabels2, int nlabels3, int *excluir, int *total);
int tipo(int diametro, int tipo, int *total);
int exclui(int *exclui, int xc, int opcao);
int vc_desenha_box(IplImage *src, OVC *blobs, int nBlobs);


#ifdef __cplusplus
}
#endif
