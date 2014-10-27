////////////////////////////////////////////////////////////////////////////////
//  Archivo: R-WlanXDecrypter.c                                               //
//  Autores: nhaalclkiemr & millanzarreta                                     //
//  Titulo: R-WlanXDecrypter                                                  //
//  Version: 1.0                                                              //
//  Fecha: 27/10/2014                                                         //
//  Descripcion: R-WlanXDecrypter es un generador de diccionarios de claves   //
//    por defecto para routers de R, una conocida compañia de Galicia. El     //
//    programa trae multiples opciones adicionales de personalizacion para    //
//    los diccionarios.                                                       //
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
#include <windows.h>

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
#define U64_FORMAT "%I64u"
#else
#define U64_FORMAT "%llu"
#endif

#define MEM_BUFFER (50*1024*1024)  //Memoria del buffer de escritura, 50MB

int time_i, time_seg;
unsigned int buffer_len;
unsigned long long int n_buff_writes;
unsigned long long int nclaves, file_pre_size;
unsigned long long int v;
HANDLE th_mostrarProceso;
int exit_th_mostrarProceso;
HANDLE proc_mutex;

typedef struct{
  unsigned int n;
  unsigned char mask_c;
  unsigned int mask_c_on;
  unsigned int h_on;
  unsigned char h;
  unsigned int r;
  unsigned long long int min;
  unsigned long long int max;
  unsigned int c;
  unsigned int q;
  unsigned int rep;
}options_type;

//Funcion para avanzar o retroceder 'avY' lineas en la consola
//En caso de retroceder limpia las lineas que retroceda
int AvYConsole(int avY) {
  int i, j, i_top;
  COORD coord;
  HANDLE h_stdout;
  CONSOLE_SCREEN_BUFFER_INFO cursor_info;
  if ((h_stdout = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {   
    return -1;
  }
  GetConsoleScreenBufferInfo(h_stdout, &cursor_info);
  coord = cursor_info.dwCursorPosition;
  coord.X = 0;
  coord.Y += avY;
  
  if (SetConsoleCursorPosition(h_stdout, coord) == 0) {
    return -1;
  }
  i_top=avY*(-1);
  for(i=0;i<i_top;i++) {
    for(j=cursor_info.dwSize.X;j>0;j--) {
      printf(" ");
    }
  }
  if (SetConsoleCursorPosition(h_stdout, coord) == 0) {
    return -1;
  }
  return 0;
}

//Funcion para revertir una string
char *str_rev(char *str) {
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

//Funcion matematica de potencia b^e
int pot(int b,int e) {
  int i, res;
  res=1;
  for(i=1;i<=e;i++) {
    res *= b;
  }
  return res;
}

//Funcion matematica de potencia b^e
unsigned long long int potull(unsigned long long int b, unsigned int e) {
  unsigned int i;
  unsigned long long int res;
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
unsigned long long int cull(char *str, int *err, int str_len) {
  unsigned long long int res;
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
      res=res+((str[i]-48)*(potull(10,j)));
      j++;
    }
  }
  return res;
}

//Convierte un array en formato 01234567 al formato 01:23:45:67 donde
//arr_hex es la salida, arr_hex_len su tamaño, arr es la entrada, sep el
//separador y offset_sep el desplazamiento del separador
void ArrayToHex(char *arr_hex, char *arr, int arr_hex_len, char sep, int offset_sep) {
  int i;
  int j = 0;
  for(i=0;i<arr_hex_len;i++) {
    if ((i%3)==2+offset_sep) {
      arr_hex[i]=sep;
    } else {
      arr_hex[i]=arr[j];
      j++;
    }
  }
}

//Convierte un numero unsigned long long int a una string
void ullToStr(unsigned long long int n, char **str, int str_size) {
  int i = 0;
  while((i<(str_size-1)) && (n>0)) {
    (*str)[i]=n%10+48;
    n=trunc(n/10);
    i++;
  }
  (*str)[i]=0;
  *str=str_rev(*str);
}

//Funcion para mostrar el proceso, se ejecutara en otro hilo
DWORD WINAPI mostrarproceso() {
  double proc;
  int dif_time = 0;
  unsigned long long int pos_act;
  double pos_act_mb, mb_rate;
  while(exit_th_mostrarProceso) {
    WaitForSingleObject(proc_mutex,INFINITE);
    pos_act=n_buff_writes*buffer_len+v;
    pos_act_mb=pos_act/1048576.0;
    proc=(pos_act/(double) file_pre_size)*100;
    dif_time=time(NULL)-time_i;
    if (dif_time) {
      mb_rate=pos_act_mb/dif_time;
    } else {
      mb_rate=pos_act_mb;
    }
    if (proc>=100) exit_th_mostrarProceso=0;
    AvYConsole(-3);
    printf("Procesados: %0.2f MB (" U64_FORMAT " bytes) <%0.2lf MB/seg> [%0.2lf%%]",pos_act_mb,pos_act,mb_rate,proc);
    ReleaseMutex(proc_mutex);
    printf("\nTiempo transcurrido: %d segundos\n\n",dif_time);
    Sleep(1000);
  }
  pos_act=n_buff_writes*buffer_len+v;
  pos_act_mb=pos_act/1048576.0;
  proc=(pos_act/(double) file_pre_size)*100;
  if (time_seg) {
    mb_rate=pos_act_mb/time_seg;
  } else {
    if (dif_time) {
      mb_rate=pos_act_mb/dif_time;
    } else {
      mb_rate=pos_act_mb;
    }
  }
  AvYConsole(-3);
  printf("Procesados: %0.2f MB (" U64_FORMAT " bytes) <%0.2lf MB/seg> [%0.2lf%c]\n",pos_act_mb,pos_act,mb_rate,proc,37);
  return(0);
}

//Funcion para mostrar el menu principal
void mostrarmenu() {
  printf("\nR-WlanXDecrypter 1.0  -  2014  nhaalclkiemr & millanzarreta\n");
  printf("\nUso: R-WlanXDecrypter <diccionario_salida> [opciones]\n");
  printf("\nOpciones:\n");
  printf("\n   -h <sep>      : Crea un diccionario hexadecimal, el separador es <sep> (si no se especifica ':' por defecto)");
  printf("\n   -n <nbits>    : Longitud de la clave WEP: 64/128/152/256/512 (128 por defecto)");
  printf("\n\nOpciones avanzadas:\n");
  printf("\n   -c <nchar>    : Numero de caracteres no fijos (8 por defecto)");
  printf("\n   -cm <decchar> : Caracter en decimal de los bytes fijos (48 por defecto)");
  printf("\n   -min <num>    : Numero inicial (del numero no fijo)");
  printf("\n   -max <num>    : Numero final (del numero no fijo)");
  printf("\n   -rep <num>    : Repetir la parte no fija <num> veces");
  printf("\n   -r            : No escribir retorno de carro CR al final de cada linea. LF en lugar de CR+LF");
  printf("\n   -m <nbytes>   : Longitud de la clave en bytes (incompatible con '-n')");
  printf("\n   -q            : Activar quiet mode (no muestra estado por pantalla)\n\n");
}

int main(int argc, char *argv[]) {
  char file[255];
  options_type opt;
  char *data;
  char *mask = NULL;
  unsigned int mask_len = 0;
  unsigned int word_size;
  char *min_str;
  int control;
  int control_c_rep_impar = 0;
  unsigned int i, j, o;
  int err;
  int time_min;
  FILE *idf;
  char *buffer;
  char *data_hex = NULL;
  char *data_hex2 = NULL;
  char *mask_hex = NULL;
  unsigned int c_hex = 0;
  unsigned int mask_hex_len = 0;
  double file_pre_size_mb;

  opt.n = 0;
  opt.mask_c = 0;
  opt.mask_c_on = 0;
  opt.h_on = 0;
  opt.h = 0;
  opt.r = 0;
  opt.min = 0;
  opt.max = 0;
  opt.c = 0;
  opt.q = 0;
  opt.rep = 0;
  time_seg=0;

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
      opt.min=cull(argv[o+1],&err,0);
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
      opt.max=cull(argv[o+1],&err,0);
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
    } else if (strcmp(argv[o],"-q")==0) {
      opt.q=1;
    } else if (strcmp(argv[o],"-rep")==0) {
      if ((o+1)>=argc) {
        printf("\nFalta el parametro de %c-rep%c\n",34,34);
        return 1;
      }
      opt.rep=cint(argv[o+1],&err,0);
      if ((err) || (opt.rep<=0)) {
        printf("\nEl parametro de %c-rep%c es incorrecto\n",34,34);
        return 1;
      }
      o++;
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
    if (opt.rep) {
      if ((opt.c*(opt.rep+1))>(opt.n*2)) {
        printf("\nParametros de %c-c%c y de %c-rep%c incorrectos\n",34,34,34,34);
        return(1);
      }
    } else if (opt.c>(opt.n*2)) {
      opt.c=opt.n*2;
    }
  } else {
    if (opt.rep) {
      if ((opt.c*(opt.rep+1))>opt.n) {
        printf("\nParametros de %c-c%c y de %c-rep%c incorrectos\n",34,34,34,34);
        return(1);
      }
    } else if (opt.c>opt.n) {
      opt.c=opt.n;
    }
  }
  if (opt.mask_c_on==0) {
    opt.mask_c=48;
  }
  if (opt.min>(potull(10,opt.c)-1)) {
    printf("\nParametro de %c-min%c incorrecto\n",34,34);
    return(1);
  }
  if (opt.max>(potull(10,opt.c)-1)) {
    printf("\nParametro de %c-max%c incorrecto\n",34,34);
    return(1);
  }

  //Operaciones previas a la escritura
  min_str = (char *) calloc(1,16);
  if (min_str==NULL) {
    printf("\nMemoria insuficiente\n");
    return -1;
  }

  ullToStr(opt.min,&min_str,16);

  data=(char *) calloc(1,opt.c+1);
  if (data==NULL) {
    printf("\nMemoria insuficiente\n");
    free(min_str);
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
    mask_hex_len=(opt.n*3-1)-opt.c*(opt.rep+1)-(opt.c*(opt.rep+1)-1)/2 + 2 - opt.r;
    c_hex=opt.c+(opt.c-1)/2;
    data_hex=(char *) calloc(1,c_hex+1);
    if (data_hex==NULL) {
      printf("\nMemoria insuficiente\n");
      free(min_str);
      free(data);
      return -1;
    }
    if ((opt.rep) && (opt.c % 2)) {
      control_c_rep_impar = 1;
      data_hex2=(char *) calloc(1,c_hex+1);
      if (data_hex2==NULL) {
        printf("\nMemoria insuficiente\n");
        free(min_str);
        free(data);
        free(data_hex);
        return -1;
      }
    }
    mask_hex=(char *) calloc(1,mask_hex_len+1);
    if (mask_hex==NULL) {
      printf("\nMemoria insuficiente\n");
      free(min_str);
      free(data);
      free(data_hex);
      if (control_c_rep_impar) free(data_hex2);
      return -1;
    }
    for(o=0;o<mask_hex_len;o++) {
      if ((o%3)==(opt.c*(opt.rep+1)%2)) {
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
    word_size = opt.n*3 + 1 - opt.r;
  } else {
    mask_len=opt.n-opt.c*(opt.rep+1)+1;
    if (opt.r==0) {
      mask_len++;
    }
    mask=(char *) calloc(1,mask_len+1);
    if (mask==NULL) {
      printf("\nMemoria insuficiente\n");
      free(min_str);
      free(data);
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
    word_size=(opt.n + 2 - opt.r);
  }
  buffer_len = (MEM_BUFFER/word_size)*word_size;
  buffer = (char *) malloc(buffer_len);
  if (buffer==NULL) {
    printf("\nMemoria insuficiente, se intentara reservar una memoria minima...");
    buffer_len = word_size;
    buffer = malloc(buffer_len);
    if (buffer==NULL) {
      printf(" FALLO!\n");  
      free(min_str);
      free(data);
      if (opt.h_on) {
        free(data_hex);
        if (control_c_rep_impar) free(data_hex2);
        free(mask_hex);
      } else {
        free(mask);
      }
      return(-1);
    } else {
      printf(" CORRECTO!\n");
    }
  }

  //Abrimos el archivo
  if ((idf=fopen(file, "wb"))==NULL) {
    printf("\nError accediendo al archivo %c%s%c\n",34,file,34);
    free(min_str);
    free(data);
    if (opt.h_on) {
      free(data_hex);
      if (control_c_rep_impar) free(data_hex2);
      free(mask_hex);
    } else {
      free(mask);
    }
    return -1;
  }

  //Establece algunos parametros
  if (opt.max) {
    nclaves=opt.max-opt.min+1;
  } else {
    nclaves=potull(10,opt.c)-opt.min;
  }
  file_pre_size=nclaves*word_size;
  file_pre_size_mb=(double) file_pre_size/1048576;

  printf("\n\nEl diccionario contendra " U64_FORMAT " claves y ocupara %0.2lfMB\n",nclaves,file_pre_size_mb);
  printf("\nGenerando diccionario... (Ctrl+C para cancelar)\n\n\n\n\n");

  exit_th_mostrarProceso=1;
  control=1;
  v=0;
  n_buff_writes=0;
  time_i=time(NULL);
  if (!(opt.q)) {
    if ((proc_mutex=CreateMutex(NULL,0,NULL))==NULL) opt.q=1;
    exit_th_mostrarProceso = 1;
    if (!(opt.q)) th_mostrarProceso = CreateThread(NULL,0,mostrarproceso,NULL,0,NULL);
  }
  //Empieza la generacion y escritura del diccionario
  if (opt.h_on) {
    //Modo: Hexadecimal
    while(control){
      ArrayToHex(data_hex,data,c_hex,opt.h,0);
      memcpy(&buffer[v],data_hex,c_hex);
      v+=c_hex;
      if (control_c_rep_impar) {
        ArrayToHex(data_hex2,data,c_hex,opt.h,-1);
        for (j=0;j<opt.rep;j++) {
          if (j%2) {
            memcpy(&buffer[v],&(opt.h),1);
            v++;
            memcpy(&buffer[v],data_hex,c_hex);
            v+=c_hex;
          } else {
            memcpy(&buffer[v],data_hex2,c_hex);
            v+=c_hex;
          }
        }
      } else {
        for (j=0;j<opt.rep;j++) {
          memcpy(&buffer[v],&(opt.h),1);
          v++;
          memcpy(&buffer[v],data_hex,c_hex);
          v+=c_hex;
        }
      }
      memcpy(&buffer[v],mask_hex,mask_hex_len);
      v+=mask_hex_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        if (!(opt.q)) {
          WaitForSingleObject(proc_mutex,INFINITE);
          n_buff_writes++;
          v=0;
          ReleaseMutex(proc_mutex);
        } else {
          v=0;
        }
      }
      if ((opt.max) && (cull(data,&err,opt.c)>=opt.max)) {
        control=0;
      }
      i=opt.c-1;
      while(data[i]>=57) {
        data[i]=48;
        if (i>0) {
          i--;
        } else {
          control=0;
        }
      }
      data[i]++;
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
    free(data_hex);
    if (control_c_rep_impar) free(data_hex2);
    free(mask_hex);
  } else {
    //Modo: ASCII
    while(control){
      memcpy(&buffer[v],data,opt.c);
      v+=opt.c;
      for (j=0;j<opt.rep;j++) {
        memcpy(&buffer[v],data,opt.c);
        v+=opt.c;
      }
      memcpy(&buffer[v],mask,mask_len);
      v+=mask_len;
      if (v>=buffer_len) {
        fwrite(buffer, 1, buffer_len, idf);
        if (!(opt.q)) {
          WaitForSingleObject(proc_mutex,INFINITE);
          n_buff_writes++;
          v=0;
          ReleaseMutex(proc_mutex);
        } else {
          v=0;
        }
      }
      if ((opt.max) && (cull(data,&err,opt.c)>=opt.max)) {
        control=0;
      }
      i=opt.c-1;
      while(data[i]>=57) {
        data[i]=48;
        if (i>0) {
          i--;
        } else {
          control=0;
        }
      }
      data[i]++;
    }
    if (v!=0) {
      fwrite(buffer, 1, v, idf);
    }
    free(mask);
  }
  time_seg=time(NULL)-time_i;

  //Finalizacion del proceso
  exit_th_mostrarProceso=0;

  if (!(opt.q)) {
    WaitForSingleObject(th_mostrarProceso,INFINITE);
    CloseHandle(th_mostrarProceso);
    CloseHandle(proc_mutex);
  }

  time_min=0;
  while (time_seg>=60) {
    time_seg-=60;
    time_min++;
  }

  printf("\nDiccionario creado correctamente como %c%s%c\n",34,file,34);
  printf("Tiempo empleado: %d minutos %d segundos\n\n",time_min,time_seg);

  //Liberacion de memoria y recursos
  free(buffer);
  fclose(idf);
  free(min_str);
  free(data);

  return(0);
}
