/*********************************************************************/
/* Synchronise system date/time with an NTP server                   */
/*   SYSTEM      : V4R5                                              */
/*   PROGRAMMER  : Dave Asta                                         */
/*   DATE-WRITTEN: 20 Oct 2021                                       */
/*   (C) 2021 Dave Asta under MIT License                            */
/*********************************************************************/
/* This CL calls program NTPCLI, which connects to an NTP server     */
/* specified here, and puts the received date and time in the *LDA   */
/* with format DDMMYYYYHHMMSS for this CL to read it and then        */
/* changes the system values QDAY, QMONTH, QYEAR and QTIME           */
/*********************************************************************/

             PGM

    /* Change this variable if you want a different NTP server */
             DCL        VAR(&NTPSERVER) TYPE(*CHAR) LEN(12) +
                          VALUE('pool.ntp.org')

    /* Call program that gets Date and Time from the NTP server */
             CALL       PGM(DASTA/NTPCLI) PARM(&NTPSERVER)

    /* Change system date */
             CHGSYSVAL  SYSVAL(QDAY) VALUE(%SUBSTRING(*LDA 1 2))
             CHGSYSVAL  SYSVAL(QMONTH) VALUE(%SUBSTRING(*LDA 3 2))
             CHGSYSVAL  SYSVAL(QYEAR) VALUE(%SUBSTRING(*LDA 7 2))
    /* Change system time */
             CHGSYSVAL  SYSVAL(QTIME) VALUE(%SUBSTRING(*LDA 9 6))

             ENDPGM
