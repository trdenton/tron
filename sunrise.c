/*

modifications for cron by troy denton 2020

original Copyright (GPL) 2004   Mike Chirico mchirico@comcast.net
Updated: Sun Nov 28 15:15:05 EST 2004

Program adapted by Mike Chirico mchirico@comcast.net

Reference:
http://prdownloads.sourceforge.net/souptonuts/working_with_time.tar.gz?download
http://www.srrb.noaa.gov/highlights/sunrise/sunrise.html


Compile:

gcc -o sunrise -Wall -W -O2 -s -pipe -lm sunrise.c

or for debug output

gcc -o sunrise -DDEBUG=1 -Wall -W -O2 -s -pipe -lm sunrise.c




This can also go in a batch job to calculate the next
20 days as follows:

#!/bin/bash
lat=39.95
long=75.15
for (( i=0; i <= 20; i++))
do
./sunrise    `date -d "+$i day" "+%Y %m %d"` $lat $long
done



 */

/* gcc -DDEBUG=1 .. */
#ifndef DEBUG
#define DEBUG 0
#endif


#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "cron.h"






double calcSunEqOfCenter(double t);


/* Convert degree angle to radians */

double  degToRad(double angleDeg)
{
    return (M_PI * angleDeg / 180.0);
}

double radToDeg(double angleRad)
{
    return (180.0 * angleRad / M_PI);
}


double calcMeanObliquityOfEcliptic(double t)
{
    double seconds = 21.448 - t*(46.8150 + t*(0.00059 - t*(0.001813)));
    double e0 = 23.0 + (26.0 + (seconds/60.0))/60.0;

    return e0;              // in degrees
}


double calcGeomMeanLongSun(double t)
{


    double L = 280.46646 + t * (36000.76983 + 0.0003032 * t);
    while( (int) L >  360 )
    {
        L -= 360.0;

    }
    while(  L <  0)
    {
        L += 360.0;

    }


    return L;              // in degrees
}






double calcObliquityCorrection(double t)
{
    double e0 = calcMeanObliquityOfEcliptic(t);


    double omega = 125.04 - 1934.136 * t;
    double e = e0 + 0.00256 * cos(degToRad(omega));
    return e;               // in degrees
}

double calcEccentricityEarthOrbit(double t)
{
    double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    return e;               // unitless
}

double calcGeomMeanAnomalySun(double t)
{
    double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    return M;               // in degrees
}


double calcEquationOfTime(double t)
{


    double epsilon = calcObliquityCorrection(t);               
    double  l0 = calcGeomMeanLongSun(t);
    double e = calcEccentricityEarthOrbit(t);
    double m = calcGeomMeanAnomalySun(t);
    double y = tan(degToRad(epsilon)/2.0);
    y *= y;
    double sin2l0 = sin(2.0 * degToRad(l0));
    double sinm   = sin(degToRad(m));
    double cos2l0 = cos(2.0 * degToRad(l0));
    double sin4l0 = sin(4.0 * degToRad(l0));
    double sin2m  = sin(2.0 * degToRad(m));
    double Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0
        - 0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;


    return radToDeg(Etime)*4.0;	// in minutes of time


}


double calcTimeJulianCent(double jd)
{

    double T = ( jd - 2451545.0)/36525.0;
    return T;
}









double calcSunTrueLong(double t)
{
    double l0 = calcGeomMeanLongSun(t);
    double c = calcSunEqOfCenter(t);

    double O = l0 + c;
    return O;               // in degrees
}



double calcSunApparentLong(double t)
{
    double o = calcSunTrueLong(t);

    double  omega = 125.04 - 1934.136 * t;
    double  lambda = o - 0.00569 - 0.00478 * sin(degToRad(omega));
    return lambda;          // in degrees
}




double calcSunDeclination(double t)
{
    double e = calcObliquityCorrection(t);
    double lambda = calcSunApparentLong(t);

    double sint = sin(degToRad(e)) * sin(degToRad(lambda));
    double theta = radToDeg(asin(sint));
    return theta;           // in degrees
}


double calcHourAngleSunrise(double lat, double solarDec)
{
    double latRad = degToRad(lat);
    double sdRad  = degToRad(solarDec);



    double HA = (acos(cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad)));

    return HA;              // in radians
}

double calcHourAngleSunset(double lat, double solarDec)
{
    double latRad = degToRad(lat);
    double sdRad  = degToRad(solarDec);


    double HA = (acos(cos(degToRad(90.833))/(cos(latRad)*cos(sdRad))-tan(latRad) * tan(sdRad)));

    return -HA;              // in radians
}


double calcJD(struct tm* t)
{
    int year = t->tm_year + 1900;
    int month = t->tm_mon + 1;
    int day = t->tm_mday;
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int A = floor(year/100);
    int B = 2 - A + floor(A/4);

    double JD = floor(365.25*(year + 4716)) + floor(30.6001*(month+1)) + day + B - 1524.5;
    return JD;
}




double calcJDFromJulianCent(double t)
{
    double JD = t * 36525.0 + 2451545.0;
    return JD;
}


double calcSunEqOfCenter(double t)
{
    double m = calcGeomMeanAnomalySun(t);

    double mrad = degToRad(m);
    double sinm = sin(mrad);
    double sin2m = sin(mrad+mrad);
    double sin3m = sin(mrad+mrad+mrad);

    double C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
    return C;		// in degrees
}






double calcSunriseUTC(double JD, double latitude, double longitude)
{

    double t = calcTimeJulianCent(JD);

    // *** First pass to approximate sunrise


    double  eqTime = calcEquationOfTime(t);
    double  solarDec = calcSunDeclination(t);
    double  hourAngle = calcHourAngleSunrise(latitude, solarDec);
    double  delta = longitude - radToDeg(hourAngle);
    double  timeDiff = 4 * delta;	// in minutes of time	
    double  timeUTC = 720 + timeDiff - eqTime;	// in minutes	
    double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0); 


    eqTime = calcEquationOfTime(newt);
    solarDec = calcSunDeclination(newt);


    hourAngle = calcHourAngleSunrise(latitude, solarDec);
    delta = longitude - radToDeg(hourAngle);
    timeDiff = 4 * delta;
    timeUTC = 720 + timeDiff - eqTime; // in minutes



    return timeUTC;
}

double calcSunsetUTC(double JD, double latitude, double longitude)
{

    double t = calcTimeJulianCent(JD);

    // *** First pass to approximate sunset


    double  eqTime = calcEquationOfTime(t);
    double  solarDec = calcSunDeclination(t);
    double  hourAngle = calcHourAngleSunset(latitude, solarDec);
    double  delta = longitude - radToDeg(hourAngle);
    double  timeDiff = 4 * delta;	// in minutes of time	
    double  timeUTC = 720 + timeDiff - eqTime;	// in minutes	
    double  newt = calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC/1440.0); 


    eqTime = calcEquationOfTime(newt);
    solarDec = calcSunDeclination(newt);


    hourAngle = calcHourAngleSunset(latitude, solarDec);
    delta = longitude - radToDeg(hourAngle);
    timeDiff = 4 * delta;
    timeUTC = 720 + timeDiff - eqTime; // in minutes

    // printf("************ eqTime = %f  \nsolarDec = %f \ntimeUTC = %f\n\n",eqTime,solarDec,timeUTC);


    return timeUTC;
}

int same_minute(struct tm* a, struct tm* b)
{
    if (!a || !b)
        return 0;
    return (a->tm_year == b->tm_year &&
            a->tm_mon == b->tm_mon &&
            a->tm_mday == b->tm_mday &&
            a->tm_hour == b->tm_hour &&
            a->tm_min == b->tm_min);
}

int _calc(struct tm* tm, double lat, double lon, double(*calcfn)(double,double,double) )
{
    struct tm midnight;
    float JD = calcJD(tm);
    struct tm localsr, ptm;
    int delta;

    memcpy(&midnight,tm,sizeof(struct tm));
    midnight.tm_hour=0;
    midnight.tm_min=0;
    midnight.tm_sec=0;
    midnight.tm_isdst=-1;

    time_t seconds = mktime(&midnight);

    if (&ptm != gmtime_r ( &seconds, &ptm ))
    {
        log_it("CRON",getpid(),"ERROR","error calculating gmtime, can't determine sun phase");
        return 0;
    }

    delta = ptm.tm_hour;

    seconds = seconds + calcfn( JD,  lat,  lon)*60;
    seconds = seconds - delta*3600;

    if (&localsr != localtime_r(&seconds,&localsr))
    {
        log_it("CRON",getpid(),"ERROR","error calculating localtime, can't determine sun phase");
        return 0;
    }

    return same_minute(tm,&localsr);
}

int sunset(struct tm* tm, double lat, double lon)
{
    return _calc(tm,lat,lon,&calcSunsetUTC);
}


int sunrise(struct tm* tm, double lat, double lon)
{
    return _calc(tm,lat,lon,&calcSunriseUTC);
}

