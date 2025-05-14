# sched

Lugar para respuestas en prosa, seguimientos con GDB y documentación del TP.

# Seguimiento de GDB para el *cambio de contexto*:

![context switch 1](<sched 1.png>)
![context switch 2](<sched 2.png>)

# Round Robin

Se implementó un metódología cíclica. Dado que si existe un proceso actual, se empieza a recorrer los procesos desde ese hasta dar la vuelta entera y llegar al mismo. Si existe un proceso en estado de **RUNNABLE** antes te llegar al actual, se lo corre.

# Prioridades

Para el de prioridades, se implementó el mismo algoritmo pero con la condición de las prioridades. Pero para esto se tuvo que definir qué:
1. Todo proceso empieza con prioridad **0** (la más alta).
2. Esa prioridad va aumentando (cada vez más baja) en base que se cambie de proceso.

En el algoritmo siempre se busca de forma cíclica el proceso **RUNNABLE** con mejor prioridad, y si no se encuentra, sigue corriendo el actual.

Otra condición que tuvimos que considerar, es que los procesos con peor prioridad nunca se iban a ejecutar. Por lo tanto, se añadió un sistema de reseteo de prioridades. Pone a todos en **0** como si fuesen recien creados, y se hace cada **10** llamados al scheduler.