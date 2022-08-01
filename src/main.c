#include <stdio.h>	// I/O
#include <stdlib.h>	// EXIT_SUCCESS/FAILURE
#include <string.h>	// kv�li strcmp();
#include <ctype.h>	// kv�li isspace();
#include <math.h>		//pro konstanty NAN a INFINITY
#include <float.h>	// nezbytn� pro DBL_DIG

// Prototypy n�kter�ch funkc�
double check_log_base(double a);
double calcNaturalLogarithm (double number, double eps);
void print_result(double result);
int check_sigdig(int sd);
double calc_epsilon (int sigdig);

// konstanty
const double IZP_E = 2.7182818284590452354;        // e
const double IZP_PI = 3.14159265358979323846;      // pi
const double IZP_2PI = 6.28318530717958647692;     // 2*pi
const double IZP_PI_2 = 1.57079632679489661923;    // pi/2
const double IZP_PI_4 = 0.78539816339744830962;    // pi/4

// Stavov� k�dy
enum state {
   S_HELP,     // 0
   S_TANH,     // 1
   S_LOGAX,    // 2
   S_WAM,      // 3
   S_WQM,      // 4
};

enum e_code {
   E_OK,          // 0
   E_WRONG_PARAM,
   E_UNKNOWN_ERROR,
};

// Chybov� zpr�vy
const char *E_MSG[] = {
	"\nVse je v poradku.\n",                                    // 0
	"\nSpatny vstupni parametr.\n",                             // 1
	"\nVyskytla se neocekavana chyba !\n",
};

typedef struct params {
   int state;
   int e_code;
   int sigdig;
   double base;
} TParams, *p_params;

typedef struct calc {
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
	"Popis parametr�:\n"
	      "\tproj2 -h\t spusti napovedu programu\n\n"
      "\tproj2 -- wam\n\tProgram nacita ze vstupu hodnoty 'x1 h1' pro vypocet vazeneho aritmetckeho pr�meru\n\n"
      "\tproj2 -- wqm\n\tProgram nacita ze vstupu hodnoty 'x1 h1' pro vypocet vazeneho kvadratickeho pr�meru\n\n"
      "\tproj2 -- tanh sigdig\n\tProgram pozaduje jako dalsi parametr pocet platnych cislic [sigdig] na kolik ma byt "
      "vypocet presny (rozsah je <1; DBL_DIB>). Nasledujici vstupni hodnoty jsou podle techto parametr� "
      "zpracovany ve vystup\n\n"
      "\tproj2 -- logax sigdig a\n\tProgram pozaduje 2 dalsi parametry. Pocet platnych cislic [sigdig] na kolik ma byt "
      "vypocet presny (rozsah je <1; DBL_DIB>) a dale zaklad logaritmu [a]. Nasledne nactene vstupni hodnoty jsou podle "
      "techto parametr� zpracovany ve vystup\n\n"
;

/*************************************************************************************************/
// DEKLARACE FUNKC�
/*************************************************************************************************/
////////////////////////////
// Funkce o�et�uj�c� mezn� hodnoty
////////////////////////////

double meanExtremValues (TCalc *p_calc) {
   double result;
   if ((p_calc->num == INFINITY) || (p_calc->weigh == INFINITY)) {
      result = INFINITY;
         if (p_calc->num == INFINITY && p_calc->weigh == INFINITY) {
            result = NAN;
         }
   }
   else {
      result = NAN;
   }
return result;
}

double tanhExtremValues (TCalc *p_calc) { // o�et�uje stavy num = INF|-INF nebo hodnoty < -100 a > 100
   double result;
   if (p_calc->num == NAN) {
      result = NAN;
   }
   else if (p_calc->num == INFINITY) {
      result = 1;
   }
   else if(p_calc->num == -INFINITY) {
      result = -1;
   }
   else if(p_calc->num > 0) {
   result = 1;
   }
   else {
   result = -1;
   }
return result;
}

double logExtremValues (TCalc *p_calc) {  // o�et�uje vstupy INF, - INF, NAN, 0 a z�porn� vstupy
   double result;
   if (p_calc->num == NAN || p_calc->num < 0) { // z�klad logaritmu > 1
         result = NAN;
      }
   else if (p_calc->base > 1) { // z�klad logaritmu > 1
      if(p_calc->num == INFINITY || p_calc->num == -INFINITY)
         result = INFINITY;
      else  // p��pad, kdy num = 0
         result = -INFINITY;
      }
   else {   // z�klad logaritmu < 1
      if(p_calc->num == INFINITY || p_calc->num == -INFINITY)
         result = -INFINITY;
      else
         result = INFINITY;
   }
return result;
}

////////////////////////////
// Inicializa�n� funkce
////////////////////////////

TCalc initCalc(TParams *p) {  // inicializace p�i spu�t�n� main(); nastav� 0 a hodnoty podle parametr�
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

TParams getParams(int argc, char *argv[]) {  //zpracov�n� vstupn�ch parametr� a vyhodnocen�
   TParams tester = {   // Inicializace struktury; defaultn� hodnoty
      .state = S_HELP,
      .e_code = E_OK,
      .sigdig = 0,
      .base = 0,
   };

   if(argc == 2) {
      if(strcmp("-h", argv[1]) == 0) {
         tester.state = S_HELP;
      }
      else if (strcmp(argv[1], "--wam") == 0) {
         tester.state = S_WAM;
      }
      else if (strcmp(argv[1],"--wqm") == 0) {
         tester.state = S_WQM;
      }
      else {
         tester.e_code = E_WRONG_PARAM;
      }
   }
   else if(argc == 3 && (strcmp(argv[1], "--tanh") == 0)) {
      int q = trunc(atoi(argv[2])); // p�evede vstup na INT a pak p��padn� zaokrouhl�
      tester.sigdig = check_sigdig(q); // kontrola rozsahu sigdig
      if(tester.sigdig) {
         tester.state = S_TANH;
      }
      else
         tester.e_code = E_WRONG_PARAM;
   }
   else if(argc == 4 && (strcmp(argv[1], "--logax") == 0)) {
      int q = trunc(atoi(argv[2])); // p�evede vstup na INT a pak p��padn� zaokrouhl�
      tester.sigdig = check_sigdig(q); // kontrola rozsahu sigdig
      tester.base = check_log_base(strtod(argv[3], NULL));  // mus� se p�etypovat na float; jinak se po��t� typ char

      if(tester.sigdig && tester.base) {
         tester.state = S_LOGAX;
      }
      else {
         tester.e_code = E_WRONG_PARAM;
      }
   }
   else
      tester.e_code = E_WRONG_PARAM;
return tester;
}

////////////////////////////
// V�po�etn� funkce
////////////////////////////

double calc_sinh (double num, double eps) {  // v�po�et hyperbolick�ho sin(x)
   double result, term;
   int counter;
   double squared2 = num * num;
   double fact;
   double result_old;

   term = num;		// 1. �len
   result = num;
   counter = 1;

   do {
      result_old = result;
      fact = 2*counter;
      term = (term * squared2)/((fact+1)*fact);
      result += term;
      counter++;
   } while ((fabs(result_old - result)) > fabs(eps*result));
return result;
}

double calc_cosh (double num, double eps) {  // v�po�et hyperbolick� cos(x)
   double result, term;
   int counter;
   double squared2 = num * num;		// dop�edu vypo��tan� x^2 aby jsem to poka�d� nemusel po��tat
   double fact;
   double result_old;

   term = 1;		// 1. �len
   result = 1;
   result_old = 0;
   counter = 1;

   do {
      result_old = result;
      fact = 2*counter;
      term = (term * squared2)/(fact*(fact-1));
      result += term;
      counter++;
   } while ((fabs(result_old - result) > eps*result));
return result;
}

double calc_epsilon (int sigdig) {  // p�evod sigding -> epsilon
   int i;
   double epsilon = 0.1;
   //epsilon = pow(0.1, sigdig+1);  // m��e se pou��vat ???
   for (i = 0; i <= sigdig+2; i++)	// p�id�no +2 - projistotu aby se neprojevila odchylka p�i d�len� do platn�ch ��slic !
      epsilon = epsilon*0.1;

return epsilon;
}

double calc_wam(TCalc *p_calc) { // v�po�et v�en�ho aritmetick�ho pr�m�ru
   double output = 0;

   p_calc->numerator += (p_calc->num * p_calc->weigh);	// meziv�po�ty (�itatel, jmenovatel) ukl�d�m do struktury a pot� s tn�m znovu po��t�m
   p_calc->denominator += p_calc->weigh;
   output = p_calc->numerator / p_calc->denominator;

return output;
}

double calc_wqm(TCalc *p_calc) {    // v�po�et v�en� kvadratick� pr�m�r
   double output = 0;

   p_calc->numerator += ((p_calc->num*p_calc->num) * p_calc->weigh);		// meziv�po�ty (�itatel, jmenovatel) ukl�d�m do struktury a pot� s tn�m znovu po��t�m
   p_calc->denominator += p_calc->weigh;
   output = sqrt(p_calc->numerator / p_calc->denominator);

return output;
}

double calcNaturalLogarithm (double number, double eps) {   // v�po�et p�irozen�ho logaritmu

   double term = 0;
   double result = 0;
   double result_old;
   int counter, divided = 0;
   double numerator;
   double mult;
   double y = 0;

	for (divided = 0; number > IZP_E; divided++) {		 // zmenseni vstupniho cisla delenim eulorovou konstantou
         number /= IZP_E;
	}
	y = (number - 1)/(number + 1);	// definice 1. �len rozvoje
	term = y;
	mult = y*y;
	//power = pow(y,2);
	numerator = term;
	result = 2*y;
	counter = 3;
	do {
		result_old = result;
		numerator = numerator*mult;
		term = term + ((numerator)/counter);
		result = 2*term;
		counter = counter + 2;
	} while (fabs(result_old - result) > fabs(eps*result));	// porovn�n� jestli bylo dosa�eno po�adovan� p�esnosti

	result += divided;		 // p�i��st po�et vyd�len� p�vodn�ho ��sla E-�kem, proto�e ln(e) = 1
return result;
}

////////////////////////////
// Kontroln� funkce
////////////////////////////

int check_sigdig(int sd) { // zkontroluje platnost sigdig <1; DBL_DIG>
   if(sd > DBL_DIG || sd < 1)
      sd = 0;
return sd;
}

double check_log_base(double base) {   // zkontroluje validnost sigdig internval "R+ / {1}"
   if(base <= 0 || base == 1)
      base = 0;
return base;
}

double check_weigh(double num) {    // kontroluje platnost v�hy pr�m�ru
   if(num < 0) // v�ha le�� v intervalu <0; +inf)
      num = NAN;
return num;
}

double checkEntry (double input_value, int q) {		// kontroluje vstupn� data (jestli se opravdu jen jedna o ��sla)
		int c;

      c = getchar();    // na��t� dal�� 1. znak po ��sle (slou�� pro kontrolu platnosti vstupu)
      if (q == 1 && (isspace(c) || c == EOF)) {;}  // jestli�e byl vstup ��slo a n�sledovala mezera/EOF => v�e OK
      else {   // chyba ! na vstupu nebylo ��slo => nastav na NaN
			if (c != isspace(c))
				ungetc(c, stdin); // mus�m vr�tit zp�t do bufferu aby to mohl p�e��st scanf("%*s"), bo v p��pad�, �e by po ��sle byl jen 1 znak,
										//tak by scanf p�esko�il dal�� vstup !
         q = scanf("%*s");
         input_value = NAN;
      }
return input_value;
}

////////////////////////////
// Funkce zaji��uj�c� proveden� po�adovan� operace
////////////////////////////

void mean_procedure (TCalc *p_calc, int average) { // funkce obhospoda�uje v�po�et pr�m�r�
   double result;
   if (p_calc->num == INFINITY || p_calc->weigh == INFINITY) {
      result = meanExtremValues(p_calc);
   }
   else {
   if(average == S_WAM)
      result = calc_wam(p_calc);
   else if (average == S_WQM)
      result = calc_wqm(p_calc);
   else
      result = 7315;
   }
   print_result(result);
}

void tanh_procedure(TCalc *p_calc) {   // funkce obhospoda�uje v�po�et tanh(x)
   double result;
   double sin;
   double cos;

   if(p_calc->num > 100 ||  p_calc->num < -100 || p_calc->num == NAN)
      result = tanhExtremValues(p_calc);
   else {
      sin = calc_sinh(p_calc->num, p_calc->eps);
      cos = calc_cosh(p_calc->num, p_calc->eps);
      result = sin / cos;
   }
   print_result(result);
}

void logax_procedure (TCalc *p_calc) { // funkce obhospoda�uje v�po�et log(a, x) [v�po�ty p�es ln(x)]
   double result;
   double lnX, lnA;

    if(p_calc->num == INFINITY ||  p_calc->num == -INFINITY || p_calc->num <= 0 || p_calc->num == NAN)
      result = logExtremValues(p_calc);
   else {
		//optimizeFraction(p_calc);
      lnX = calcNaturalLogarithm(p_calc->num, p_calc->eps);
      lnA = calcNaturalLogarithm(p_calc->base, p_calc->eps);
      result = lnX / lnA;
   }
   print_result(result);
}

////////////////////////////
// Dal�� podp�rn� funkce
////////////////////////////

void end_of_mean() {// kontroluje jestli p�i v�po�tu pr�m�r� bylo zachov�n pom�r hodnota:v�ha a nen� lich� po�et ��slic
   double result = NAN;
   print_result(result);
}

void print_result(double result) {
   printf("%.10e\n", result);
}

void print_error(int ecode) { // tiskne chybov� hl�en�
   fprintf(stderr, "%s", E_MSG[ecode]);
}
/*************************************************************************************************/
// HLAVN� T�LO PROGRAMU
/*************************************************************************************************/

int main(int argc, char *argv[])
{
   double input_value = 0;
   int i = 0, q = 0;

   TParams params = getParams(argc, argv);   // zpracov�n� parametr�
   TParams *p_params = &params;  // ukazatel na strukturu
   if (params.e_code != 0) {   // jestli�e se vyskytla chyba -> schluss
      print_error(params.e_code);
      return EXIT_FAILURE;
   }
   if (params.state == S_HELP) {
      printf("%s", HELPMSG);
      return EXIT_SUCCESS;
   }

   TCalc calc = initCalc(p_params); // inicializace pomocn� struktury a p�ed�n� vstupn�ch parametr�
   TCalc *p_calc = &calc;

   while((q = scanf("%lf", &input_value)) != EOF) {   // �te ��seln� vstup
		input_value = checkEntry(input_value, q);
      if(params.state == S_WAM) {
         if(i % 2 == 0) {  // prvn� vstup je ��slo, dal�� je v�ha; a� pak za�ni po��tat pr�m�r
            calc.num = input_value;
            i++;
            continue;
         }
         calc.weigh = check_weigh(input_value);
         i++;
         mean_procedure(p_calc, p_params->state);
      }
      else if(params.state == S_WQM) {
         if(i % 2 == 0) {  // prvn� vstup je ��slo, dal�� je v�ha; a� pak za�ni po��tat pr�m�r
            calc.num = input_value;
            i++;
            continue;
         }
         calc.weigh = check_weigh(input_value);
         i++;
         mean_procedure(p_calc, p_params->state);
      }
      else if(params.state == S_TANH) {
         calc.num = input_value;
			tanh_procedure(p_calc);
      }
      else if(params.state == S_LOGAX) {
         calc.num = input_value;
         logax_procedure(p_calc);
      }
      else {
         print_error(params.e_code = E_UNKNOWN_ERROR);
         return EXIT_FAILURE;
      }
   }
   if (i % 2 != 0)   // ne�pln� vstup p�i v�po�tu pr�m�r� (vypi� NaN)
      end_of_mean();

return EXIT_SUCCESS;
}
