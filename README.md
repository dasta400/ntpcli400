# Synchronise OS/400 V4R5 system date/time with an NTP server

OS/400 V4R5 lacks an NTP client, so here is one.

I followed the specifications provided in [RFC 5905](https://www.rfc-editor.org/info/rfc5905) for the **N**etwork **T**ime **P**rotocol Version 4.

NTP servers don't return the UTC offset, hence the QUTCOFFSET value is not changed. On my AS/400, I set it up as +00:00

## Components 

### NTPCLIC (CLP)

- It calls NTPCLI, passing *pool.ntp.org* as a parameter. 
- It reads the new date and time from the *LDA, that was changed by NTPCLI.
- changes (CHGSYSVAL) the values of QDAY, QMONTH, QYEAR and QTIME.

### NTPCLI (C)

It connects to an NTP server address received by parameter, retrieves the date and time and puts it in the *LDA, in the format DDMMYYYYHHMMSS.

There is a constant *DEBUG*, that can be set to 1 or 0. If set to 1, it will output some statements to know what the program is doing (e.g. create socket, connect to socket, close socket, retrieved date/time). If set to 0, no output will be printed out.

I have it as 1 on my system, so after the Job Scheduler has run it, I can see the spool file.

