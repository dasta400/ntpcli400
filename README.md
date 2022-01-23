# Synchronise OS/400 V4R5 system date/time with an NTP server

OS/400 V4R5 lacks an NTP client, so here is one.

I followed the specifications provided in [RFC 5905](https://www.rfc-editor.org/info/rfc5905) for the **N**etwork **T**ime **P**rotocol Version 4.

NTP servers don't return the UTC offset, hence the *QUTCOFFSET* value still needs to be maintaned manually. Or use https://github.com/PoC-dev/as400-autodst

This new version consists only of a C program, that connects to the specified NTP server and changes the system values QDAY, QMONTH, QCENTURY, QYEAR, QTIME.

Call it with:

```CALL PGM(NTPCLI) PARM('<ntpserver_ip/hostname>' '<debug_val>')```

where: 

* *<ntpserver_ip/hostname>* is for example pool.ntp.org
* *<debug_val>* if set to 1, it will output some statements to know what the program is doing (e.g. create socket, connect to socket, close socket, retrieved date/time). If set to 0, no output will be printed out.
