# shell

### Búsqueda en $PATH
1. ¿Cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?

    La función ‘execve(2)’ tiene la siguiente firma:

    `int execve(const char* pathname, char *const _Nullable argv[], char *const _Nullable envp[]);`

    Donde pathname indica, generalmente, el nombre de un ejecutable binario (relativo o absoluto), argv es un arreglo con los argumentos (terminado en NULL), y envp es un arreglo con las variables de entorno (terminado en NULL).

    La familia de wrappers ‘exec(3)’ se construye sobre esta función, y proporcionan funcionalidades especializadas en cada caso. Se pueden clasificar por sus sufijos:

    1. **l**, sus firmas son variádicas en lugar de recibir arreglos de strings. 
    2. **v**, respeta la firma original de execve y requiere que se pasen arreglos de strings tanto para los argumentos como para las variables de entorno.     
    3. **e**, recibe variables de entorno, ya sea como argumentos variádicos o como arreglos de strings.    
    4. **p**, hace búsquedas en la variable de entorno PATH.

2. ¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?

    Todos los wrappers de la familia exec(3) pueden fallar, y también puede fallar execve(2), en dicho caso devuelven -1 y cambian el valor de **errno** para indicar el tipo de error. En el caso de la shell implementada en el TP, se muestra por consola (stderror) el mensaje "execvp falló", seguido del error descrito por errno (haciendo uso de **perror**).

---

### Procesos en segundo plano
1. ¿Por qué es necesario el uso de señales?

    El uso de señales para el manejo de procesos en background, como SIGCHLD, permite definir funciones especiales, llamadas *handlers*, para controlar el fin del ciclo de vida del proceso que se estaba corriendo. No usar signals requeriría métodos menos prácticos, como esperar a una nueva entrada por línea de comandos antes de controlar el fin de los procesos en background, que podrían ser limitantes, lentos, o traer fugas de memoria.

---

### Flujo estándar
1. Investigar el significado de 2>&1, explicar cómo funciona su forma general.

    La forma general se la puede denotar como: **fd_1>&fd_2** donde se está redireccionando lo del **fd_1** a lo del **fd_2**. En el ejemplo mostrado **2>&1** estamos indicando que el file descriptor 2 que es de **stderr**, lo estamos direccionando al file descriptor 1 es de **stdout**. Entonces, estaríamos mostrando por pantalla el error.

2. Mostrar qué sucede con la salida de cat out.txt en el ejemplo.  

    `ls -C /home /noexiste >out.txt 2>&1`

    Mediante este orden de argumentos estamos ejecutando primero **>out.txt** que nos dice por default que salida estándar va a ser direccionada al out.txt. Luego sigue **2>&1** que estamos diciendo la salida de error está siendo direccionada a la salida estándar, pero la estándar original ya fue direccionada. Por lo tanto, en el archivo va a estar tanto el error por el directorio inexistente como la salida del ls en el home.


3. Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1). 

    `ls -C /home /noexiste 2>&1 >out.txt`

    A diferencia de esto, primero estamos indicando que el error va a ir al stdout original para luego direccionar la salida al archivo. Como primero se ejecutó lo anterior, se va a mostrar primero por pantalla el error y en el archivo solo va a estar la salida del ls.

---

### Tuberías múltiples
1. Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe

    El exit code reportado por la shell es el del último comando ejecutado en el pipe. Esto significa que si se ejecuta un pipe con varios comandos, el exit code será el de la última llamada a **exec(3)** en la cadena de comandos. Por ejemplo, si se ejecuta `ls | grep txt | wc`, el exit code será el de `wc`, que es el último comando en la string.

2. ¿Cambia en algo?

    En la implementación de la shell, el exit code reportado es el de la última llamada a exec(3) en el pipe, en bash el comportamiento por defecto es el mismo. Sin embargo, también se puede utilizar `set -o pipefail` para que el exit code reportado sea el de la primera llamada a exec(3) que falle. Esto permite detectar errores en cualquier parte de la cadena de comandos, no solo en el último comando.

3. ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

    Si alguno de los comandos en un pipe falla, el exit code reportado por la shell será el de la última llamada a exec(3) en el pipe, pero seguirá ejecutando los siguientes comandos en la cadena. El comportamiento de bash es similar con la posibilida d de usar `set -o pipefail` para que el exit code reportado sea el de la primera llamada a exec(3) que falle.

    Si alguno de los comando en un pipe falla, el exit code reportado igual será el de la última llamada a exec(3) en el pipe, mostrando el mensaje de error correspondiente y continuando con la ejecución de los siguientes comandos. Por ejemplo, si se ejecuta `ls | cat noexiste | wc`, el comando `cat` fallará porque el archivo no existe, pero la shell seguirá ejecutando `wc` y reportará el exit code de `wc`, que será 0 si no hubo errores en su ejecución.
    Por ejemplo:

    ```
    $ ls | grep txt | wc
    0 0 0
    $ echo $?
    0
    $ ls | cat noexiste | wc
    cat: noexiste: No such file or directory
    0 0 0
    $ echo $?
    0
    ```
---

### Variables de entorno temporarias
1. ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

    Las variables de entorno temporales deben afectar el entorno **solo del proceso a ejecutarse**, si se les asigna un valor antes de hacer un fork, entonces estarían afectando el entorno de la propia shell.

2. En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).

- ¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.

    La diferencia sustancial entre usar setenv(3) y pasarle variables de entorno a algún wrapper de exec(3) que acepte variables de entorno (exec[e]) es que setenv **añade variables al entorno actual**, o las sobreescribe si se le indica, mientras que exec[e] **construye un entorno con las variables de entorno pasadas**. Esto es, si se usa una función que no sea exec[e], el entorno se hereda de la shell, y setenv solamente añade o sobreescribe variables actuales; construir un arreglo con las variables de entorno pasadas de la forma convencional y pasarlo a exec[e] haría que todas las variables de entorno que no están siendo pasadas, pero que pertenecen al entorno de la shell, no formarán parte del entorno del proceso hijo, haciendo que **las metodologías no produzcan el mismo resultado**.

- Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

    La función setenv modifica la variable global **environ***, que guarda las variables de entorno del proceso actual, así que sería posible añadir las variables pasadas con setenv (o manualmente) y luego pasarle environ a exec[e]. Esto, sin embargo, no tiene mucho sentido, ya que otros wrappers utilizan directamente esta variable global.

---

### Pseudo-variables
1. Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en bash (u otra terminal similar).

    `$RANDOM`: genera un número pseudo aleatorio entre 0 y 32767.

    Ejemplo de uso: `echo $RANDOM`

    `$SECONDS`: devuelve el tiempo en segundos desde que se inició la shell.

    Ejemplo de uso: `echo $SECONDS`

    `$$`: devuelve el PID de la shell actual.

    Ejemplo de uso: `echo $$`

---

### Comandos built-in
1. ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? 
 
    El comando **pwd**, que imprime el directorio actual en el que está la shell, puede ser implementado sin ser built-in. Esto se debe a que solo necesita cargar el directorio actual a un buffer con la función getcwd y luego imprimir dicho buffer; por otro lado, **cd** no podría implementarse como un binario, ya que hace uso de la función chdir, que cambia el directorio **del proceso actual**, por lo que el proceso en el que corre la shell debería invocarlo, y no podría delegarlo a un proceso hijo.

2. ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)

    Similar a true y false, incluir pwd como built-in se reduce mayormente a una cuestión de eficiencia; si fuera un binario, la shell debería forkearse y llamar a execv, construyendo un nuevo proceso sobre el hijo, para una función tan simple como imprimir el directorio actual. Es más eficiente simplemente replicar la funcionalidad de pwd desde el proceso de la shell a invocarlo como un binario.

---

### Historial
1. ¿Cuál es la función de los parámetros MIN y TIME del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a MIN en 1 y a TIME en 0?

    

---
