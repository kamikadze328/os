# os
The lab for the os course itmo  
Разработать программу на языке С, которая осуществляет следующие действия 

Создает область памяти размером 81 мегабайт, начинающихся с адреса 0x3CCCDEDE (если возможно) при помощи mmap=(malloc, mmap) заполненную случайными числами /dev/urandom в 17 потоков. Используя системные средства мониторинга определите адрес начала в адресном пространстве процесса и характеристики выделенных участков памяти. Замеры виртуальной/физической памяти необходимо снять:

1. До аллокации  
2. После аллокации  
3. После заполнения участка данными 
4. После деаллокации  

Записывает область памяти в файлы одинакового размера 8 мегабайт с использованием nocache=(блочного, некешируемого) обращения к диску. Размер блока ввода-вывода 2 байт. Преподаватель выдает в качестве задания последовательность записи/чтения блоков seq=(последовательный, заданный  или случайный)

Генерацию данных и запись осуществлять в бесконечном цикле.

В отдельных 6 потоках осуществлять чтение данных из файлов и подсчитывать агрегированные характеристики данных - max =(сумму, среднее значение, максимальное, минимальное значение).

Чтение и запись данных в/из файла должна быть защищена примитивами синхронизации flock=(futex, cv, sem, flock).

По заданию преподавателя изменить приоритеты потоков и описать изменения в характеристиках программы. 
