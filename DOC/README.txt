***************************************************************************
PLAN DE BECARIOS DE SEGURIDAD EN COMPUTO
CURSO LLAMADAS AL SISTEMA
PROYECTO FINAL
ROOTKIT - MODULO DE KERNEL


VERSION 1.0
Autor :	Javier Ulises Santillan Arenas
	bec_jsantillan@seguridad.unam.mx
	(fuentes diversas)
/***********************************************************************/

ANTECEDENTES

Los rootkits son herramientas desarrolladas para ocultar la presencia de procesos,
archivos y conexiones en un sistema que ha sido comprometido. Los especialistas de
seguridad requieren conocer el comportamiento interno de estas herramientas a fin 
de poder desarrollar los conocimientos y práctica necesaria para identicar y conte-
ner este tipo de amenazas.

Existen al menos dos tipos de rootkits:

*Aplicación
*Kernel

La primera generación de rootkit, los de aplicaciín se basan en modificar el com-
portamiento de los comandos (ls, ps, netstat, etc) que los usuarios utilizan para 
visualizar los procesos, conexiones y archivos, de manera que se oculten al usua-
rio, sin embargo, este tipo de herramientas resultan muy limitadas y fáciles de 
identificar.

La última generación de estas herramientas, se basan en integrar este comportamien-
to en el kernel del sistema operativo con los mismos resultados de ocultar la infor-
mación, siendo estos últimos más efectivos, ya que no se modifican los archivos y
solo un especialista en seguridad experimentado podría identificar realizando una
revisión de seguridad al sistema.



OBJETIVO

El objetivo del proyecto es utilizar los conocimientos del curso de llamadas al sis-
tema para desarrollar un programa rootkit de kernel, que secuestre algunas llamadas
de la tabla de syscall (sys_call_table) a fin de modificar el comportamiento del 
sistema y oculte información sobre los procesos, archivos y conexiones.


COMPILACION DEL MODULO
Para la compilacion e instalacion se piden 3 datos de entrada:
	- Ruta de los fuentes del Kernel
	- Modo de ejecucion del modulo
	- Numero de dispositivo
	- Nombre de dispositivo

Para la ruta del kernel: se debe indicar donde se encuentran los archivos fuentes 
compilados del kernel (probado con 2.6.18.4).

Modo de ejecucion :si se indica como valor "1" MODO DEBUG, al ejecutarse el modulo
se registrada todo lo que se oculta en /var/log/messages. Esto tiene la finalidad de
entender todo lo que esta haciendo internamente el programa, asi como tambien fue de
utilidad al encontrar errores en lso argumentos durante la programacion del modulo ^^.

Numero de dispositivo: Indica el numero que identificara al conjunto de dispositivos
que podran funcionar para el paso de parametros de reconfiguracion del rootkit.

Nombre de dispositivo: Indica el nombre del modulo que se creara en /dev  para el paso de 
parametros de reconfiguracion. Posteriormente (como se explica mas abajo), el usuario
puede crear mas dispositivos indicando el numero del parametro anterior.

**********************************************************
DESEMPAQUETADO DEL PROYECTO
	# tar -zxvf lsproc.tar.gz

	ESTRUCTURA CREADA:

	../DOC/README.txt
	../SRC/lsproc.c
	../SRC/compilar
**********************************************************


**********************************************************
INSTALACION DEL MODULO
**********************************************************
Para la instalacion del modulo debemos de ejecutar el siguiente comando:

	# insmod lsproc.ko procs=1050,1030,2020 fs=hack,/tmp/dir 

**********************************************************
4.3 ELIMINACION DEL MODULO
**********************************************************
Para eliminar el modulo se ejecuta la siguiente instruccion:
	
	# rmmod lsproc.ko
	

**********************************************************
CARACTERISTICAS DEL PROYECTO
**********************************************************

* El proyecto fue desarrollado en lenguaje C, pra un kernel Linux 2.6.18.4
* El proyecto acepta como argumentos los valores que permiten modificar el comportamiento
  del rootkit a traves de un redireccionamiento a un dipositivo o mediante argumentos al
  momento de insertar el modulo.

  # insmod  lsproc.ko procs=1050,1030,2020 fs=hack,dir


Los parametros son:

	a) procs.- PID del proceso o la lista de los procesos que deberán ocultar el rootkit
		   al comando ps.
	b) fs.-	   Archivos o directorios que deberán ocultarse al ejecutarse el comando ls.

Los dos parametros pueden aceptar un valor o una lista de valores separados por comas.

**********************************************************
OCULTAR ARCHIVOS Y DIRECTORIOS
**********************************************************

El programa contiene la función de ocultar archivos y directorio. Se basa en el hecho de que
la llamada al sistema getdents utilizada por ls fue 'modificada' de modo que manipule los
datos que lista en el contenido de los directorios. 
	
LIMITANTE : Es importante mencionar que solo hace la comparacion por cadena del archivo a
	    ocultar, es decir, ocultara todo archivo que tenga el nombre de la cadena en todo
	    el sistema de archivos.

***********************************************************
OCULTAR PROCESOS
***********************************************************

El rootkit es capaz de ocultar los procesos que se le pasen como argumentos por los metodos
antes mencionados. El funcionamiento se basa igualmente en la manipulacion de la llamada
getdents, en la cual se maneja una lista de procesos la cual va comparando y cuando se 
encuentre el pid a ocultar, sa salta de la lista y continua con el siguiente.
	
***********************************************************
RECONFIGURACION DE PARAMETROS A TRAVES DE UN DISPOSITIVO   
***********************************************************

El proyecto tiene la capacidad de modificar el comportamiento del rootkit a traves del paso
de parametros a traves de redireccionamiento de un dispositivo (/dev/xxx) por ejemplo 
(/dev/ttymia).
Durante la fase de compilacion del programa se pide un numero de dispositivo y al nombre del
mismo. Con esto, por default se creara en el directorio /dev, el nombre asignado al mismo.
La finalidad de que se pida un numero para un dispositivo es para que se tenga la capacidad
de crear dispositivos adicionales definidos por el usuario, indicando especificamente el
numero de dispositivo. Esto quiere decir que si en el programa de instalacion se ingresa
el numero 999 para un dispositivo, y como nombre 'rootkit', se ejecuta internamente:
	# mknod /dev/rootkit c 999 0
Asi, si el usuario quiere crear otro, lo debera hacer bajo el numero especificado:
	# mknod /dev/tty2 c 999 0
	# mknod /dev/tty3 c 999 0
	# mknod /dev/ttyn c 999 0

Para pasar parametros se indica de la siguiente manera:
Procesos y Archivos	:	# echo "procs=9999,8888,7777 fs=archivo1,archivo2" > /dev/XXX
Solo proceso		:	# echo "procs=9999,8888,7777" > /dev/XXX
Solo archivos		:      	# echo "fs=archivo1,archivo2"

*********************************************************
LIMITACIONES DEL PROYECTO
*********************************************************
******************
PASO DE ARGUMENTOS

Es muy importante que para el correcto funcionamiento del modulo, todos los argumentos que se 
pasen se hagan sin espacios, esto es: 
	ERROR	:	insmod procs = 1234, 456, 67  fs = archivox
	OK	:	insmod procs=1234,456,67 fs=archivox,archivoy
        ERROR   :       echo "procs = 1234, 456, 67  fs = archivox" > /dev/XX
        OK      :       echo "procs=1234,456,67 fs=archivox,archivoy" > /dev/XX

En algunos casos (aleatorio), si el modulo se esta ejecutando y en ese momento se instala un
dispositivo (memoria USB, etc), el Sistema puede quedar inhabilitado.

*) 	En ocasiones (aleatorio), si el modulo se ha cargado y descargado varias veces, puede llegar
	un momento en que no deje cargarlo mas, y para solucionarlo se debe reiniciar la maquina.
*)	En el momento de la compilacion del programa, si los datos pasados como argumentos (ruta
	de fuentes del kernel, numero  y nombre de dispositivo) no son correctos, la compilacion
	marcara un error. El programa no valida que estos sean correctos.

********************
ALCANCE DEL PROYECTO

No se aceptan rutas absolutas para la ocultacion de directorios. Como se menciono antes, la
ocultacion de archivos y directorios funciona de manera relativa pero en todo el sistema de
archivos, esto es, al pasarle como parametro un archivo, ocultara el nombre de ese archivo
en todo el sistema de archivos.

Puede ocultarse solo el modulo mismo con la limitante que se necesitaria reiniciar la maquina
para descargarlo. Si no se oculta, se puede cargar y descargar.

