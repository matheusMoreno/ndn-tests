/*
 * Para compilar o codigo corretamente, colocar na linha de comando as
 * seguintes bibliotecas:
 *
 * g++ -std=c++11 -o <nome_arquivo> consumer_timer.cpp -lndn-cxx -lboost_system
 *
 */

#include <iostream>
#include <fstream>
#include <random>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <ndn-cxx/face.hpp>

#define NUMERO_CONTEUDOS      10000
#define NUMERO_PEDIDOS         1250
#define NUMERO_DIVISOES         100

#define TEMPO_ESPERA             10

std::string nomeBase = "/ufrj/gta/moreno/"; // Nome base dos interesses
std::ofstream resultados;
int contador = 0, nConteudo;
struct timespec inicio, fim;
double tempo;
double medias[NUMERO_DIVISOES] = {0.0};
int qtdePedidos[NUMERO_DIVISOES] = {0};
bool success = false;

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

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 9999);

  for (contador = 0; contador < NUMERO_PEDIDOS; contador++) {
    nConteudo = dis(gen);

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
  resultados.open("resultados-uni.txt");
  resultados << "Valores medios (em ms): \n\n";

  for (contador = 0; contador < NUMERO_DIVISOES; contador++) {
    resultados << contador * NUMERO_DIVISOES << "," << qtdePedidos[contador] << "," << medias[contador] << "," << medias[contador] / qtdePedidos[contador] << std::endl;
  }

  std::cout << "Teste terminado. Fechando o arquivo de resultados..." << std::endl;
  resultados.close();

  return 0;
}
