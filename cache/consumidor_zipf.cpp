/*
 * Para compilar o codigo corretamente, colocar na linha de comando as
 * seguintes bibliotecas:
 *
 * g++ -std=c++11 -o <nome_arquivo> consumer_timer.cpp -lndn-cxx -lboost_system -lm
 *
 */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <time.h>
#include <math.h>
#include <ndn-cxx/face.hpp>

#define NUMERO_CONTEUDOS      10000
#define NUMERO_PEDIDOS         1000
#define NUMERO_DIVISOES         100
#define ALPHA_ZIPF              0.8

#define TEMPO_ESPERA              5

#define  FALSE          0       // Boolean false
#define  TRUE           1       // Boolean true

std::string nomeBase = "/ufrj/gta/moreno/"; // Nome base dos interesses
std::ofstream resultados;
int contador = 0, nConteudo;
struct timespec inicio, fim;
double tempo;
double medias[NUMERO_DIVISOES] = {0.0};
int qtdePedidos[NUMERO_DIVISOES] = {0};
bool success = false;

int      zipf(double alpha, int n);  // Returns a Zipf random variable
double   rand_val(int seed);         // Jain's RNG

int zipf(double alpha, int n)
{
  static int first = TRUE;      // Static first time flag
  static double c = 0;          // Normalization constant
  double z;                     // Uniform random number (0 < z < 1)
  double sum_prob;              // Sum of probabilities
  double zipf_value;            // Computed exponential value to be returned
  int    i;                     // Loop counter

  // Compute normalization constant on first call only
  if (first == TRUE)
  {
    for (i=1; i<=n; i++)
      c = c + (1.0 / pow((double) i, alpha));
    c = 1.0 / c;
    first = FALSE;
  }

  // Pull a uniform random number (0 < z < 1)
  do
  {
    z = rand_val(0);
  }
  while ((z == 0) || (z == 1));

  // Map z to the value
  sum_prob = 0;
  for (i=1; i<=n; i++)
  {
    sum_prob = sum_prob + c / pow((double) i, alpha);
    if (sum_prob >= z)
    {
      zipf_value = i;
      break;
    }
  }

  // Assert that zipf_value is between 1 and N
  assert((zipf_value >=1) && (zipf_value <= n));

  return(zipf_value);
}

double rand_val(int seed)
{
  const long  a =      16807;  // Multiplier
  const long  m = 2147483647;  // Modulus
  const long  q =     127773;  // m div a
  const long  r =       2836;  // m mod a
  static long x;               // Random int value
  long        x_div_q;         // x divided by q
  long        x_mod_q;         // x modulo q
  long        x_new;           // New x value

  // Set the seed if argument is non-zero and then return zero
  if (seed > 0)
  {
    x = seed;
    return(0.0);
  }

  // RNG using integer arithmetic
  x_div_q = x / q;
  x_mod_q = x % q;
  x_new = (a * x_mod_q) - (r * x_div_q);
  if (x_new > 0)
    x = x_new;
  else
    x = x_new + m;

  // Return a random value between 0.0 and 1.0
  return((double) x / m);
}

namespace ndn
{
  class Consumidor : noncopyable
  {
  public:
    void
    pedir(int numeroInteresse)
    {
      Interest interesse;
      interesse.setInterestLifetime(20_s);
      interesse.setMustBeFresh(true);

      // Damos um nome para o interesse com um numero gerado randomicamente
      interesse.setName((nomeBase + to_string(numeroInteresse)).c_str());

      m_face.expressInterest(interesse,
                             bind(&Consumidor::seDado, this, _1, _2),
                             bind(&Consumidor::seNack, this, _1, _2),
                             bind(&Consumidor::seTimeout, this, _1));

      std::cout << "Enviando " << interesse << std::endl;
      m_face.processEvents();
    }

  private:
    Face m_face;

    void
    seDado(const Interest& interesse, const Data& dado) {
      std::cout << dado << std::endl;
      success = true;
    }

    void
    seNack(const Interest& interesse, const lp::Nack& nack) {
      std::cout << "Recebido um Nack para o interesse " << interesse
                << " pelo motivo: " << nack.getReason() << std::endl;
    }

    void
    seTimeout(const Interest& interesse) {
      std::cout << "Timeout para o interesse " << interesse << std::endl;
    }

  }; // class Consumidor
} // namespace ndn

int
main(int argc, char **argv)
{
  ndn::Consumidor consumidor;

  rand_val((int) time(NULL));

  for (contador = 0; contador < NUMERO_PEDIDOS; contador++) {
    nConteudo = (int) zipf(ALPHA_ZIPF, NUMERO_CONTEUDOS);

    while (!success) {
      clock_gettime(CLOCK_MONOTONIC, &inicio);

      try {
        consumidor.pedir(nConteudo);
      }
      catch (const std::exception& e) {
        std::cerr << "ERRO: " << e.what() << std::endl;
        break;
      }
    }

    // Calculando o atraso
    clock_gettime(CLOCK_MONOTONIC, &fim);
    tempo = (fim.tv_sec - inicio.tv_sec) * 1000.0;
    tempo += (fim.tv_nsec - inicio.tv_nsec) / 1000000.0;

    // Salvando os resultados
    std::cout << "Atraso: " << tempo << "ms" << std::endl;
    medias[nConteudo % NUMERO_DIVISOES] += tempo;
    qtdePedidos[nConteudo % NUMERO_DIVISOES] += 1;

    sleep(TEMPO_ESPERA);
    success = false;
  }

  // Salvando os resultados no arquivo
  resultados.open("resultados-zipf.txt");
  resultados << "Valores medios (em ms): \n\n";

  for (contador = 0; contador < NUMERO_DIVISOES; contador++) {
    resultados << contador * NUMERO_DIVISOES << "," << qtdePedidos[contador] << "," << medias[contador] << "," << medias[contador] / qtdePedidos[contador] << std::endl;
  }

  std::cout << "Teste terminado. Fechando o arquivo de resultados..." << std::endl;
  resultados.close();

  return 0;
}
