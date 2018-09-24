/*
 * Para compilar o codigo corretamente, colocar na linha de comando as
 * seguintes bibliotecas:
 *
 * g++ -std=c++11 -o <nome_arquivo> consumer_timer.cpp -lndn-cxx -lboost_system
 *
 */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <ndn-cxx/face.hpp>

#define NUMERO_CONTEUDOS      10000
#define NUMERO_PEDIDOS         1000
#define NUMERO_DIVISOES         100

#define TEMPO_ESPERA             10

std::string nomeBase = "/ufrj/gta/moreno/"; // Nome base dos interesses
std::ofstream resultados;
int contador = 0, nConteudo;
struct timespec inicio, fim;
double tempo;
double medias[NUMERO_DIVISOES] = {0.0};
int qtdePedidos[NUMERO_DIVISOES] = {0};

double unifd(int min, int max); // Returns a discrete uniform RV
long rand_vald(int seed);       // Jain's RNG to return a discrete number

double unifd(int min, int max)
{
  int    z;                     // Uniform random integer number
  int    unif_value;            // Computed uniform value to be returned

  // Pull a uniform random integer
  z = rand_vald(0);

  // Compute uniform discrete random variable using inversion method
  unif_value = z % (max - min + 1) + min;

  return(unif_value);
}

long rand_vald(int seed)
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

  // Return a random integer number
  return(x);
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

  rand_vald((int) time(NULL));

  for (contador = 0; contador < NUMERO_PEDIDOS; contador++) {
    nConteudo = (int) unifd(0, NUMERO_CONTEUDOS - 1);
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    try {
      consumidor.pedir(nConteudo);
    }
    catch (const std::exception& e) {
      std::cerr << "ERRO: " << e.what() << std::endl;
      break;
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
  }

  // Salvando os resultados no arquivo
  resultados.open("resultados.txt");
  resultados << "Valores medios (em ms): \n\n";

  for (contador = 0; contador < NUMERO_DIVISOES; contador++) {
    resultados << contador * NUMERO_DIVISOES << "," << qtdePedidos[contador] <<
               "," << medias[contador] << "," << medias[contador] / qtdePedidos[contador] << std::endl;
  }

  std::cout << "Teste terminado. Fechando o arquivo de resultados..." << std::endl;
  resultados.close();

  return 0;
}
