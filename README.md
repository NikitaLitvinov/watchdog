# Watchdog

Приложение `watchdog` запускает другое подконтрольное приложение, если приложение завершиться, то взводиться таймер по истечению которого произойдет перезапуск приложения.
Остановить запуск приложения возможно путем нажатия `Ctrl+c`.
На вход приложения принимает служебное слово `start` и далее запускаемое приложение с аргументами.

Пример запуска:
```shell
./watchdog start htop
```

## Сборка

В качестве системы сборки используется CMake.
Для сборки выполните команду:
```shell
mkdir cmake-build && cd cmake-build && cmake .. && make
```

Исполняемый файл - `bin/watchdog`.

## Пример работы
Запустим приложение lighttpd:
```shell
./watchdog start /home/nik/projects/lighttpd-1.4.65/src/lighttpd -m /home/nik/projects/lighttpd-1.4.65/src/.libs -D -f /home/nik/projects/lighttpd-1.4.65/lighttpd.conf
Watchdog start.
Process /home/nik/projects/lighttpd-1.4.65/src/lighttpd start.
2022-07-18 12:24:05: (server.c.1588) server started (lighttpd/1.4.65)
```
Для завершения подконтрольного процесса нажмем `Ctrl+c`:
```shell
# Ctrl+c
2022-07-18 12:25:08: (server.c.2097) server stopped by UID = 0 PID = 0
Process /home/nik/projects/lighttpd-1.4.65/src/lighttpd finish.
Process will be restart with timeout 5.
To stop restart and exit press Ctrl-C.
Timer create success!
Signal fd create success!
Poll create success!
Start timeout - 5 sec.
Timeout!
Finish timeout.
Process /home/nik/projects/lighttpd-1.4.65/src/lighttpd start.
2022-07-18 12:25:12: (server.c.1588) server started (lighttpd/1.4.65)
```
Как видим после завершения подконтрольный процесс перезапустился.

Теперь оставим полностью выполнение `watchdog`:
```shell
# Останавливаем lighttpd
^C2022-07-18 12:27:00: (server.c.1019) [note] graceful shutdown started
2022-07-18 12:27:00: (server.c.2097) server stopped by UID = 0 PID = 0
Process /home/nik/projects/lighttpd-1.4.65/src/lighttpd finish.
Process will be restart with timeout 5 sec.
To stop restart and exit press Ctrl-C.
Timer create success!
Signal fd create success!
Poll create success!
Start timeout - 5 sec.
# Нажимаем Ctrl-C
^CCatch signal Interrupt
Finish timeout.
Watchdog finish.
```
Процесс полностью завершился.

## Исходное ТЗ
Написать приложение – watchdog для других приложений:

* Запуск произвольного ПО через watchdog
* Контроль состояния процесса, при необходимости – перезапуск с таймаутом 5 сек
* Предпочтительно использовать timerfd и signalfd
* Язык – С, использовать Linux api
* Предусмотреть возможность остановки процесса
* Приложение должно стабильно работать с lighttpd
* Пример запуска watchdog start lighttpd

 