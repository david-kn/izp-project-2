#include <stdio.h>  // I/O
#include <stdlib.h> // EXIT_SUCCESS/FAILURE
#include <string.h> // kvůli strcmp();
#include <ctype.h>  // kvůli isspace();
#include <math.h>   // pro konstanty NAN a INFINITY
#include <float.h>  // nezbytné pro DBL_DIG

// Prototypy některých funkcí
double check_log_base(double a);
double calcNaturalLogarithm(double number, double eps);
void print_result(double result);
int check_sigdig(int sd);
double calc_epsilon(int sigdig);

// konstanty
const double IZP_E = 2.7182818284590452354;     // e
const double IZP_PI = 3.14159265358979323846;   // pi
const double IZP_2PI = 6.28318530717958647692;  // 2*pi
const double IZP_PI_2 = 1.57079632679489661923; // pi/2
const double IZP_PI_4 = 0.78539816339744830962; // pi/4

// Stavové kódy
enum state
{
   S_HELP,  // 0
   S_TANH,  // 1
   S_LOGAX, // 2
   S_WAM,   // 3
   S_WQM,   // 4
};

enum e_code
{
   E_OK, // 0
   E_WRONG_PARAM,
   E_UNKNOWN_ERROR,
};

// Chybové zprávy
const char *E_MSG[] = {
    "\nVse je v poradku.\n",        // 0
    "\nSpatny vstupni parametr.\n", // 1
    "\nVyskytla se neocekavana chyba !\n",
};

typedef struct params
{
   int state;
   int e_code;
   int sigdig;
   double base;
} TParams, *p_params;

typedef struct calc
{
   double subtotal;
   double numerator;
   double denominator;
   double num;
   double weigh;
   double eps;
   double base;
   int sign;
} TCalc, *p_calc;

const char *HELPMSG =
    "IZP - Projekt c. 2 - Projekt c. 2 - Iteracni vypocty\n"
    "Autor: David Konar\n"
    "E-mail: xkonar07@stud.fit.vutbr.cz\n"
    "Pouziti:\n"
    "\tproj2 -h\t spusti napovedu programu\n"
    "\tproj2 -- wam\n"
    "\tproj2 -- wqm\n"
    "\tproj2 -- tanh sigdig\n"
    "\tproj2 -- logax sigdig a\n"
    "Popis parametrů:\n"
    "\tproj2 -h\t spusti napovedu programu\n\n"
    "\tproj2 -- wam\n\tProgram nacita ze vstupu hodnoty 'x1 h1' pro vypocet vazeneho aritmetckeho průmeru\n\n"
    "\tproj2 -- wqm\n\tProgram nacita ze vstupu hodnoty 'x1 h1' pro vypocet vazeneho kvadratickeho průmeru\n\n"
    "\tproj2 -- tanh sigdig\n\tProgram pozaduje jako dalsi parametr pocet platnych cislic [sigdig] na kolik ma byt "
    "vypocet presny (rozsah je <1; DBL_DIB>). Nasledujici vstupni hodnoty jsou podle techto parametrů "
    "zpracovany ve vystup\n\n"
    "\tproj2 -- logax sigdig a\n\tProgram pozaduje 2 dalsi parametry. Pocet platnych cislic [sigdig] na kolik ma byt "
    "vypocet presny (rozsah je <1; DBL_DIB>) a dale zaklad logaritmu [a]. Nasledne nactene vstupni hodnoty jsou podle "
    "techto parametrů zpracovany ve vystup\n\n";

/*************************************************************************************************/
// DEKLARACE FUNKCÍ
/*************************************************************************************************/
////////////////////////////
// Funkce oąetřující mezní hodnoty
////////////////////////////

double meanExtremValues(TCalc *p_calc)
{
   double result;
   if ((p_calc->num == INFINITY) || (p_calc->weigh == INFINITY))
   {
      result = INFINITY;
      if (p_calc->num == INFINITY && p_calc->weigh == INFINITY)
      {
         result = NAN;
      }
   }
   else
   {
      result = NAN;
   }
   return result;
}

double tanhExtremValues(TCalc *p_calc)
{ // oąetřuje stavy num = INF|-INF nebo hodnoty < -100 a > 100
   double result;
   if (p_calc->num == NAN)
   {
      result = NAN;
   }
   else if (p_calc->num == INFINITY)
   {
      result = 1;
   }
   else if (p_calc->num == -INFINITY)
   {
      result = -1;
   }
   else if (p_calc->num > 0)
   {
      result = 1;
   }
   else
   {
      result = -1;
   }
   return result;
}

double logExtremValues(TCalc *p_calc)
{ // oąetřuje vstupy INF, - INF, NAN, 0 a záporné vstupy
   double result;
   if (p_calc->num == NAN || p_calc->num < 0)
   { // základ logaritmu > 1
      result = NAN;
   }
   else if (p_calc->base > 1)
   { // základ logaritmu > 1
      if (p_calc->num == INFINITY || p_calc->num == -INFINITY)
         result = INFINITY;
      else // případ, kdy num = 0
         result = -INFINITY;
   }
   else
   { // základ logaritmu < 1
      if (p_calc->num == INFINITY || p_calc->num == -INFINITY)
         result = -INFINITY;
      else
         result = INFINITY;
   }
   return result;
}

////////////////////////////
// Inicializační funkce
////////////////////////////

TCalc initCalc(TParams *p)
{ // inicializace při spuątěné main(); nastaví 0 a hodnoty podle parametrů
   TCalc calc = {
       .subtotal = 0,
       .numerator = 0,
       .denominator = 0,
       .num = 0,
       .weigh = 0,
       .eps = calc_epsilon(p->sigdig),
       .base = p->base,
       .sign = -1,
   };
   return calc;
}

TParams getParams(int argc, char *argv[])
{ //zpracování vstupních parametrů a vyhodnocení
   TParams tester = {
       // Inicializace struktury; defaultní hodnoty
       .state = S_HELP,
       .e_code = E_OK,
       .sigdig = 0,
       .base = 0,
   };

   if (argc == 2)
   {
      if (strcmp("-h", argv[1]) == 0)
      {
         tester.state = S_HELP;
      }
      else if (strcmp(argv[1], "--wam") == 0)
      {
         tester.state = S_WAM;
      }
      else if (strcmp(argv[1], "--wqm") == 0)
      {
         tester.state = S_WQM;
      }
      else
      {
         tester.e_code = E_WRONG_PARAM;
      }
   }
   else if (argc == 3 && (strcmp(argv[1], "--tanh") == 0))
   {
      int q = trunc(atoi(argv[2]));    // převede vstup na INT a pak případně zaokrouhlí
      tester.sigdig = check_sigdig(q); // kontrola rozsahu sigdig
      if (tester.sigdig)
      {
         tester.state = S_TANH;
      }
      else
         tester.e_code = E_WRONG_PARAM;
   }
   else if (argc == 4 && (strcmp(argv[1], "--logax") == 0))
   {
      int q = trunc(atoi(argv[2]));                        // převede vstup na INT a pak případně zaokrouhlí
      tester.sigdig = check_sigdig(q);                     // kontrola rozsahu sigdig
      tester.base = check_log_base(strtod(argv[3], NULL)); // musí se přetypovat na float; jinak se počítá typ char

      if (tester.sigdig && tester.base)
      {
         tester.state = S_LOGAX;
      }
      else
      {
         tester.e_code = E_WRONG_PARAM;
      }
   }
   else
      tester.e_code = E_WRONG_PARAM;
   return tester;
}

////////////////////////////
// Výpočetní funkce
////////////////////////////

double calc_sinh(double num, double eps)
{ // výpočet hyperbolického sin(x)
   double result, term;
   int counter;
   double squared2 = num * num;
   double fact;
   double result_old;

   term = num; // 1. člen
   result = num;
   counter = 1;

   do
   {
      result_old = result;
      fact = 2 * counter;
      term = (term * squared2) / ((fact + 1) * fact);
      result += term;
      counter++;
   } while ((fabs(result_old - result)) > fabs(eps * result));
   return result;
}

double calc_cosh(double num, double eps)
{ // výpočet hyperbolický cos(x)
   double result, term;
   int counter;
   double squared2 = num * num; // dopředu vypočítané x^2 aby jsem to pokaľdé nemusel počítat
   double fact;
   double result_old;

   term = 1; // 1. člen
   result = 1;
   result_old = 0;
   counter = 1;

   do
   {
      result_old = result;
      fact = 2 * counter;
      term = (term * squared2) / (fact * (fact - 1));
      result += term;
      counter++;
   } while ((fabs(result_old - result) > eps * result));
   return result;
}

double calc_epsilon(int sigdig)
{ // převod sigding -> epsilon
   int i;
   double epsilon = 0.1;
   //epsilon = pow(0.1, sigdig+1);  // můľe se pouľívat ???
   for (i = 0; i <= sigdig + 2; i++) // přidáno +2 - projistotu aby se neprojevila odchylka při dělení do platných číslic !
      epsilon = epsilon * 0.1;

   return epsilon;
}

double calc_wam(TCalc *p_calc)
{ // výpočet váľeného aritmetického průměru
   double output = 0;

   p_calc->numerator += (p_calc->num * p_calc->weigh); // mezivýpočty (čitatel, jmenovatel) ukládám do struktury a poté s tním znovu počítám
   p_calc->denominator += p_calc->weigh;
   output = p_calc->numerator / p_calc->denominator;

   return output;
}

double calc_wqm(TCalc *p_calc)
{ // výpočet váľený kvadratický průměr
   double output = 0;

   p_calc->numerator += ((p_calc->num * p_calc->num) * p_calc->weigh); // mezivýpočty (čitatel, jmenovatel) ukládám do struktury a poté s tním znovu počítám
   p_calc->denominator += p_calc->weigh;
   output = sqrt(p_calc->numerator / p_calc->denominator);

   return output;
}

double calcNaturalLogarithm(double number, double eps)
{ // výpočet přirozeného logaritmu

   double term = 0;
   double result = 0;
   double result_old;
   int counter, divided = 0;
   double numerator;
   double mult;
   double y = 0;

   for (divided = 0; number > IZP_E; divided++)
   { // zmenseni vstupniho cisla delenim eulorovou konstantou
      number /= IZP_E;
   }
   y = (number - 1) / (number + 1); // definice 1. člen rozvoje
   term = y;
   mult = y * y;
   //power = pow(y,2);
   numerator = term;
   result = 2 * y;
   counter = 3;
   do
   {
      result_old = result;
      numerator = numerator * mult;
      term = term + ((numerator) / counter);
      result = 2 * term;
      counter = counter + 2;
   } while (fabs(result_old - result) > fabs(eps * result)); // porovnání jestli bylo dosaľeno poľadované přesnosti

   result += divided; // přičíst počet vydělení původního čísla E-čkem, protoľe ln(e) = 1
   return result;
}

////////////////////////////
// Kontrolní funkce
////////////////////////////

int check_sigdig(int sd)
{ // zkontroluje platnost sigdig <1; DBL_DIG>
   if (sd > DBL_DIG || sd < 1)
      sd = 0;
   return sd;
}

double check_log_base(double base)
{ // zkontroluje validnost sigdig internval "R+ / {1}"
   if (base <= 0 || base == 1)
      base = 0;
   return base;
}

double check_weigh(double num)
{               // kontroluje platnost váhy průměru
   if (num < 0) // váha leľí v intervalu <0; +inf)
      num = NAN;
   return num;
}

double checkEntry(double input_value, int q)
{ // kontroluje vstupní data (jestli se opravdu jen jedna o čísla)
   int c;

   c = getchar(); // načítá daląí 1. znak po čísle (slouľí pro kontrolu platnosti vstupu)
   if (q == 1 && (isspace(c) || c == EOF))
   {
      ;
   } // jestliľe byl vstup číslo a následovala mezera/EOF => vąe OK
   else
   { // chyba ! na vstupu nebylo číslo => nastav na NaN
      if (c != isspace(c))
         ungetc(c, stdin); // musím vrátit zpět do bufferu aby to mohl přečíst scanf("%*s"), bo v případě, ľe by po čísle byl jen 1 znak,
                           //tak by scanf přeskočil daląí vstup !
      q = scanf("%*s");
      input_value = NAN;
   }
   return input_value;
}

////////////////////////////
// Funkce zajią»ující provedení poľadované operace
////////////////////////////

void mean_procedure(TCalc *p_calc, int average)
{ // funkce obhospodařuje výpočet průměrů
   double result;
   if (p_calc->num == INFINITY || p_calc->weigh == INFINITY)
   {
      result = meanExtremValues(p_calc);
   }
   else
   {
      if (average == S_WAM)
         result = calc_wam(p_calc);
      else if (average == S_WQM)
         result = calc_wqm(p_calc);
      else
         result = 7315;
   }
   print_result(result);
}

void tanh_procedure(TCalc *p_calc)
{ // funkce obhospodařuje výpočet tanh(x)
   double result;
   double sin;
   double cos;

   if (p_calc->num > 100 || p_calc->num < -100 || p_calc->num == NAN)
      result = tanhExtremValues(p_calc);
   else
   {
      sin = calc_sinh(p_calc->num, p_calc->eps);
      cos = calc_cosh(p_calc->num, p_calc->eps);
      result = sin / cos;
   }
   print_result(result);
}

void logax_procedure(TCalc *p_calc)
{ // funkce obhospodařuje výpočet log(a, x) [výpočty přes ln(x)]
   double result;
   double lnX, lnA;

   if (p_calc->num == INFINITY || p_calc->num == -INFINITY || p_calc->num <= 0 || p_calc->num == NAN)
      result = logExtremValues(p_calc);
   else
   {
      //optimizeFraction(p_calc);
      lnX = calcNaturalLogarithm(p_calc->num, p_calc->eps);
      lnA = calcNaturalLogarithm(p_calc->base, p_calc->eps);
      result = lnX / lnA;
   }
   print_result(result);
}

////////////////////////////
// Daląí podpůrné funkce
////////////////////////////

void end_of_mean()
{ // kontroluje jestli při výpočtu průměrů bylo zachován poměr hodnota:váha a není lichý počet číslic
   double result = NAN;
   print_result(result);
}

void print_result(double result)
{
   printf("%.10e\n", result);
}

void print_error(int ecode)
{ // tiskne chybové hláąení
   fprintf(stderr, "%s", E_MSG[ecode]);
}
/*************************************************************************************************/
// HLAVNÍ TĚLO PROGRAMU
/*************************************************************************************************/

int main(int argc, char *argv[])
{
   double input_value = 0;
   int i = 0, q = 0;

   TParams params = getParams(argc, argv); // zpracování parametrů
   TParams *p_params = &params;            // ukazatel na strukturu
   if (params.e_code != 0)
   { // jestliľe se vyskytla chyba -> schluss
      print_error(params.e_code);
      return EXIT_FAILURE;
   }
   if (params.state == S_HELP)
   {
      printf("%s", HELPMSG);
      return EXIT_SUCCESS;
   }

   TCalc calc = initCalc(p_params); // inicializace pomocné struktury a předání vstupních parametrů
   TCalc *p_calc = &calc;

   while ((q = scanf("%lf", &input_value)) != EOF)
   { // čte číselný vstup
      input_value = checkEntry(input_value, q);
      if (params.state == S_WAM)
      {
         if (i % 2 == 0)
         { // první vstup je číslo, daląí je váha; aľ pak začni počítat průměr
            calc.num = input_value;
            i++;
            continue;
         }
         calc.weigh = check_weigh(input_value);
         i++;
         mean_procedure(p_calc, p_params->state);
      }
      else if (params.state == S_WQM)
      {
         if (i % 2 == 0)
         { // první vstup je číslo, daląí je váha; aľ pak začni počítat průměr
            calc.num = input_value;
            i++;
            continue;
         }
         calc.weigh = check_weigh(input_value);
         i++;
         mean_procedure(p_calc, p_params->state);
      }
      else if (params.state == S_TANH)
      {
         calc.num = input_value;
         tanh_procedure(p_calc);
      }
      else if (params.state == S_LOGAX)
      {
         calc.num = input_value;
         logax_procedure(p_calc);
      }
      else
      {
         print_error(params.e_code = E_UNKNOWN_ERROR);
         return EXIT_FAILURE;
      }
   }
   if (i % 2 != 0) // neúplný vstup při výpočtu průměrů (vypią NaN)
      end_of_mean();

   return EXIT_SUCCESS;
}
