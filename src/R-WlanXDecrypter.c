////////////////////////////////////////////////////////////////////////////////
//  Archivo: R-WlanXDecrypter.c                                               //
//  Autor: nhaalclkiemr                                                       //
//  Titulo: R-WlanXDecrypter                                                  //
//  Version: 0.8                                                              //
//  Fecha: 06/10/2010                                                         //
//  Descripcion: R-WlanXDecrypter es un generador de diccionarios de claves   //
//    por defecto para routers de R, una conocida compañia de Galicia. El     //
//    programa trae multiples opciones adicionales de personalizacion para    //
//    los diccionarios, que no son necesarias saber utilizar.                 //
//  Licencia: Este programa es software libre; puedes redistribuirlo y/o      //
//    modificarlo bajo los terminos de la Licencia Publica General GNU (GPL)  //
//    publicada por la Free Software Foundation; en su version numero 2, o    //
//    (bajo tu criterio) la ultima version. Mas informacion:                  //
//    http://www.fsf.org/copyleft/gpl.txt                                     //
//  No se ofrece garantía de ningún tipo                                      //
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MEM_BUFFER (50*1024*1024)  //Memoria del buffer de escribura, 50MB


char *strrev(char *str) {
  int i, j, temp;
  i=0;
  j=strlen(str) - 1;
  while(i < j) {
    temp = str[i];
    str[i++] = str[j];
    str[j--] = temp;
  }
  return str;
}

typedef struct{
  unsigned int n;
  unsigned char mask_c;
  int mask_c_on;
  int h_on;
  unsigned char h;
  int r;
  double min;
  double max;
  unsigned int c;
}options_type;

//Función matematica de potencia b^e
int pot(int b,int e) {
int i, res;
res=1;
for(i=1;i<=e;i++) {
  res *= b;
}
return res;
}

//Función matematica de potencia b^e
double potdoub(double b, int e) {
int i;
double res;
res=1;
for(i=1;i<=e;i++) {
  res *= b;
}
return res;
}

//Convertir string en int, err devuelve 1 si hubo algun error, str_len
//puede ser definido como 0 si la string termina en caracter nulo '\0'
int cint(char *str, int *err, int str_len) {
int res;
int i,j;
*err=0;
i=0;
j=0;
res=0;
if (!(str_len)) {
  str_len=strlen(str);
}
for(i=str_len-1;i>=0;i--) {
  if ((str[i]<48) || (str[i]>57)) {
    *err=1;
    return 0;
  } else {
    res=res+((str[i]-48)*(pot(10,j)));
    j++;
  }
}
return res;
}

//Convertir string en int, err devuelve 1 si hubo algun error, str_len
//puede ser definido como 0 si la string termina en caracter nulo '\0'
double cdoub(char *str, int *err, int str_len) {
double res;
int i,j;
*err=0;
i=0;
j=0;
res=0;
if (!(str_len)) {
  str_len=strlen(str);
}
for(i=str_len-1;i>=0;i--) {
  if ((str[i]<48) || (str[i]>57)) {
    *err=1;
    return 0;
  } else {
    res=res+((str[i]-48)*(potdoub(10,j)));
    j++;
  }
}
return res;
}

//Convierte un array en formato 01234567 al formato 01:23:45:67 donde
//arr_hex es la salida, arr_hex_len su tamaño, arr es la entrada y sep el
//separador
void ArrayToHex(char *arr_hex, char *arr, int arr_hex_len, char sep) {
  int i;
  int j = 0;
  for(i=0;i<arr_hex_len;i++) {
    if ((i%3)==2) {
      arr_hex[i]=sep;
    } else {
      arr_hex[i]=arr[j];
      j++;
    }
  }
}

//Convierte un numero double a una string
void doubToStr(double n, char **str, int str_size) {
  int i = 0;
  while((i<(str_size-1)) && (n>0)) {
    (*str)[i]=fmod(n,10)+48;
    n=trunc(n/10);
    i++;
  }
  (*str)[i]=0;
  *str=strrev(*str);
}

//Funcion para mostrar el menu principal
void mostrarmenu() {
  printf("\nR-WlanXDecrypter 0.8  -  2010  nhaalclkiemr\n");
  printf("\nUso: R-WlanXDecrypter <diccionario_salida> [opciones]\n");
  printf("\nOpciones:\n");
  printf("\n   -h <sep>      : Crea un diccionario hexadecimal, el separador es <sep> (si no se especifica ':' por defecto)");
  printf("\n   -n <nbits>    : Longitud de la clave WEP: 64/128/152/256/512 (128 por defecto)");
  printf("\n\nOpciones avanzadas:\n");
  printf("\n   -c <nchar>    : Numero de caracteres no fijos (8 por defecto)");
  printf("\n   -cm <decchar> : Caracter en decimal de los bytes fijos (48 por defecto)");
  printf("\n   -min <num>    : Numero inicial (del numero no fijo)");
  printf("\n   -max <num>    : Numero final (del numero no fijo)");
  printf("\n   -r            : No escribir retorno de carro CR al final de cada linea. LF en lugar de CR+LF");
  printf("\n   -m <nbytes>   : Longitud de la clave en bytes (incompatible con '-n')\n\n");
}

int main(int argc, char *argv[]) {

char file[255];
options_type opt;
char *data;
char *mask;
unsigned int mask_len;
char *min_str;
int control;
int i, o, err, v;
int time_i, time_seg, time_min;
FILE *idf;
char *buffer;
int word_size, buffer_len;
char *data_hex, *mask_hex;
int c_hex, mask_hex_len;

opt.n = 0;
opt.mask_c = 0;
opt.mask_c_on = 0;
opt.h_on = 0;
opt.h = 0;
opt.r = 0;
opt.min = 0;
opt.max = 0;
opt.c = 0;
min_str = (char *) calloc(1,16);
if (min_str==NULL) {
  printf("\nMemoria insuficiente\n");
  return -1;
}

//sin argumentos o con -? muestra el menu
if (argc<=1) {
  mostrarmenu();
  return 0;
}
if (strcmp(argv[1],"-?")==0) {
    mostrarmenu();
    return 0;
}

//Establecer archivo de salida
strcpy(file,argv[1]);

//Establecer opciones de los argumentos
for(o=2;o<argc;o++) {
  if (strcmp(argv[o],"-?")==0) {
    mostrarmenu();
    return 0;
  } else if (strcmp(argv[o],"-n")==0) {
    if (opt.n!=0) {
      printf("\nParametro %c-n%c repetido o especificado junto con el parámetro incompatible %c-m%c\n",34,34,34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      opt.n=13;  //(128-24)/8;
    } else {      
      opt.n=cint(argv[o+1],&err,0);
      if (err) {
        opt.n=13;  //(128-24)/8;
      } else {
        if (!((opt.n==64) || (opt.n==128) || (opt.n==152) || (opt.n==256) || (opt.n==512))) {
          printf("\nEl parametro de %c-n%c es incorrecto\n",34,34);
          return 1;
        }
        opt.n=(opt.n-24)/8;
        o++;
      }
    }
  } else if (strcmp(argv[o],"-m")==0) {
    if (opt.n!=0) {
      printf("\nParametro %c-m%c repetido o especificado junto con el parámetro incompatible %c-n%c\n",34,34,34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      printf("\nFalta el parametro de %c-m%c\n",34,34);
      return 1;
    }
    opt.n=cint(argv[o+1],&err,0);
    if ((err) || (opt.n<=0)) {
      printf("\nEl parametro de %c-m%c es incorrecto\n",34,34);
      return 1;
    }
    o++;
  } else if (strcmp(argv[o],"-h")==0) {
    if (opt.h_on) {
      printf("\nParametro %c-h%c repetido\n",34,34);
      return 1;
    }
    opt.h_on=1;
    opt.h=':';
    if (!((o+1)>=argc)) {
      if ((cint(argv[o+1],&err,0)<0) || (cint(argv[o+1],&err,0)>255) || ((cint(argv[o+1],&err,0)>47) && (cint(argv[o+1],&err,0)<58))) {
        printf("\nEl parametro de %c-h%c debe estar dentro del rango [0-47][58-255]\n",34,34);
        return 1;
      }
      opt.h=cint(argv[o+1],&err,0);
      if (err) {
        opt.h=':';
      } else {
        o++;
      }
    }
  } else if (strcmp(argv[o],"-min")==0) {
    if (opt.min!=0) {
      printf("\nParametro %c-min%c repetido\n",34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      printf("\nFalta el parametro de %c-min%c\n",34,34);
      return 1;
    }
    if (strlen(argv[o+1])>15) {
      printf("\nEl parametro de %c-min%c no puede exceder de 15 digitos\n",34,34);
      return 1;
    }
    opt.min=cdoub(argv[o+1],&err,0);
    if (err) {
      printf("\nEl parametro de %c-min%c es incorrecto\n",34,34);
      return 1;
    }
    if (opt.max!=0) {
      if (opt.min>opt.max) {
        printf("\nEl parametro de %c-min%c debe ser menor o igual que el de %c-max%c\n",34,34,34,34);
        return 1;
      }
    }
    o++;
  } else if (strcmp(argv[o],"-max")==0) {
    if (opt.max!=0) {
      printf("\nParametro %c-max%c repetido\n",34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      printf("\nFalta el parametro de %c-max%c\n",34,34);
      return 1;
    }
    if (strlen(argv[o+1])>15) {
      printf("\nEl parametro de %c-max%c no puede exceder de 15 digitos\n",34,34);
      return 1;
    }
    opt.max=cdoub(argv[o+1],&err,0);
    if ((err) || (opt.max==0)) {
      printf("\nEl parametro de %c-max%c es incorrecto\n",34,34);
      return 1;
    }
    if (opt.min!=0) {
      if (opt.min>opt.max) {
        printf("\nEl parametro de %c-min%c debe ser menor o igual que el de %c-max%c\n",34,34,34,34);
        return 1;
      }
    }
    o++;
  } else if (strcmp(argv[o],"-c")==0) {
    if (opt.c!=0) {
      printf("\nParametro %c-c%c repetido\n",34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      printf("\nFalta el parametro de %c-c%c\n",34,34);
      return 1;
    }
    opt.c=cint(argv[o+1],&err,0);
    if ((err) || (opt.c<=0)) {
      printf("\nEl parametro de %c-c%c es incorrecto\n",34,34);
      return 1;
    }
    o++;
  } else if (strcmp(argv[o],"-cm")==0) {
    if (opt.mask_c_on!=0) {
      printf("\nPrametro %c-cm%c repetido\n",34,34);
      return 1;
    }
    if ((o+1)>=argc) {
      printf("\nFalta el parametro de %c-cm%c\n",34,34);
      return 1;
    }
    if ((cint(argv[o+1],&err,0)<0) || (cint(argv[o+1],&err,0)>255)) {
      printf("\nEl parametro de %c-cm%c debe estar dentro del rango [0-255]\n",34,34);
      return 1;
    }
    opt.mask_c=cint(argv[o+1],&err,0);
    if (err) {
      printf("\nEl parametro de %c-cm%c es incorrecto\n",34,34);
      return 1;
    }
    opt.mask_c_on=1;
    o++;
  } else if (strcmp(argv[o],"-r")==0) {
    opt.r=1;
  } else {
    printf("\nParametro %c%s%c no reconocido\n",34,argv[o],34);
    return 1;
  }
}

//Opciones por defecto y ajuste
if (opt.n==0) {
  opt.n=13;  //(128-24)/8
}
if (opt.c==0) {
  opt.c=8;
}
if (opt.h_on) {
  if (opt.c>(opt.n*2)) {
    opt.c=opt.n*2;
  }
} else {
  if (opt.c>opt.n) {
    opt.c=opt.n;
  }
}
if (opt.mask_c_on==0) {
  opt.mask_c=48;
}

//Abrimos el archivo
if ((idf=fopen(file, "wb"))==NULL) {
  printf("\nError accediendo al archivo %c%s%c\n",34,file,34);
  return -1;
}

//Operaciones previas a la escritura

doubToStr(opt.min,&min_str,16);

data=(char *) calloc(1,opt.c+1);
if (data==NULL) {
  printf("\nMemoria insuficiente\n");
  return -1;
}

for(o=0;o<opt.c;o++) {
  if (o<(opt.c-strlen(min_str))) {
    data[o]=48;
  } else {
    data[o]=min_str[o-(opt.c-strlen(min_str))];
  }
}
if (opt.h_on) {
  mask_hex_len=((opt.n-opt.c)+((opt.c+1)/2))*3+(opt.c%2)+2-opt.r;
  c_hex=opt.c+(opt.c-1)/2;
  data_hex=(char *) calloc(1,c_hex+1);
  mask_hex=(char *) calloc(1,mask_hex_len+1);
  mask=(char *) calloc(1,mask_hex_len+1);
  for(o=0;o<mask_hex_len;o++) {
    if ((o%3)==(opt.c%2)) {
      mask_hex[o]=opt.h;
    } else {
      mask_hex[o]=opt.mask_c;
    }
  }
  if (opt.r) {
    mask_hex[mask_hex_len-1]=10;
  } else {
    mask_hex[mask_hex_len-2]=13;
    mask_hex[mask_hex_len-1]=10;
  }
  word_size=(c_hex+mask_hex_len);
} else {
  mask_len=opt.n-opt.c+1;
  if (opt.r==0) {
    mask_len++;
  }
  mask=(char *) calloc(1,mask_len+1);
  if (mask==NULL) {
    printf("\nMemoria insuficiente\n");
    return -1;
  }
  for(o=0;o<mask_len;o++) {
    mask[o]=opt.mask_c;
  }
  if (opt.r) {
    mask[mask_len-1]=10;
  } else {
    mask[mask_len-2]=13;
    mask[mask_len-1]=10;
  }
  word_size=(opt.c+mask_len);
}
buffer_len = (MEM_BUFFER/word_size)*word_size;
buffer = (char *) malloc(buffer_len);
if (buffer==NULL) {
  printf("\nMemoria insuficiente, se intentara reservar una memoria minima...");
  buffer_len = word_size;
  buffer = malloc(buffer_len);
  if (buffer==NULL) {
    printf(" FALLO!\n");  
    return(-1);
  } else {
    printf(" CORRECTO!\n");
  }
}


if (buffer==NULL) {
  printf("\nMemoria insuficiente, se intentara reservar una memoria minima...");
  buffer_len = word_size;
  buffer = malloc(buffer_len);
  if (buffer==NULL) {
    printf(" FALLO!\n");  
    return(-1);
  } else {
    printf(" CORRECTO!\n");
  }
}

//Empieza la generacion y escritura del diccionario

printf("\nGenerando diccionario...\n\n");

time_i=time(NULL);
if (opt.h_on) {
  if (opt.max) {
    //Modo: Hexadecimal, con limite maximo establecido (-max)
    control=1;
    v=0;
    while(control){
      ArrayToHex(data_hex,data,c_hex,opt.h);
      memcpy(&buffer[v],data_hex,c_hex);
      v+=c_hex;
      memcpy(&buffer[v],mask_hex,mask_hex_len);
      v+=mask_hex_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        v=0;
      }
      if (cdoub(data,&err,opt.c)>=opt.max) {
        control=0;
      }
      i=opt.c-1;
      data[i]++;
      while(data[i]>=58) {
        data[i]=48;
        if (i>0) {
          i--;
          if (data[i]==opt.h) i--;
          data[i]++;
        } else {
          control=0;
        }
      }
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
  } else {
    //Modo: Hexadecimal, sin limite maximo
    control=1;
    v=0;
    while(control){
      ArrayToHex(data_hex,data,c_hex,opt.h);
      memcpy(&buffer[v],data_hex,c_hex);
      v+=c_hex;
      memcpy(&buffer[v],mask_hex,mask_hex_len);
      v+=mask_hex_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        v=0;
      }
      i=opt.c-1;
      data[i]++;
      while(data[i]>=58) {
        data[i]=48;
        if (i>0) {
          i--;
          if (data[i]==opt.h) i--;
          data[i]++;
        } else {
          control=0;
        }
      }
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
  }
  free(data_hex);
  free(mask_hex);
} else {
  if (opt.max) {
    //Modo: ASCII, con limite maximo establecido (-max)
    control=1;
    v=0;
    while(control){
      memcpy(&buffer[v],data,opt.c);
      v+=opt.c;
      memcpy(&buffer[v],mask,mask_len);
      v+=mask_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        v=0;
      }
      if (cdoub(data,&err,opt.c)>=opt.max) {
        control=0;
      }
      i=opt.c-1;
      data[i]++;
      while(data[i]>=58) {
        data[i]=48;
        if (i>0) {
          i--;
          data[i]++;
        } else {
          control=0;
        }
      }
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
  } else {
    //Modo: ASCII, sin limite maximo
    control=1;
    v=0;
    while(control){
      memcpy(&buffer[v],data,opt.c);
      v+=opt.c;
      memcpy(&buffer[v],mask,mask_len);
      v+=mask_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        v=0;
      }
      i=opt.c-1;
      data[i]++;
      while(data[i]>=58) {
        data[i]=48;
        if (i>0) {
          i--;
          data[i]++;
        } else {
          control=0;
        }
      }
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
  }
}
time_seg=time(NULL)-time_i;
//Finalizacion del proceso

time_min=0;
while (time_seg>=60) {
  time_seg-=60;
  time_min++;
}

printf("\nDiccionario creado correctamente como '%s'\n",file);
printf("Tiempo necesario: %d minutos %d segundos\n",time_min,time_seg);

//Liberacion de memoria y cierre de archivos
free(buffer);
fclose(idf);
free(min_str);
free(mask);
free(data);
return(0);

}
