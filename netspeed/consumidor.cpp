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

#define CONTEUDO_0                    "/ufrj/gta/moreno/controle"
#define CONTEUDO_1                    "/ufrj/gta/moreno/computacao"
#define CONTEUDO_2                    "/ufrj/gta/moreno/eletronica"
#define CONTEUDO_3                    "/ufrj/gta/moreno/fisica"

#define NUMERO_REPETICOES             25
#define NUMERO_TESTES                  4

std::ofstream resultados;
int contador = 0, teste = 0, i;
double vect_resultados[NUMERO_REPETICOES];
struct timespec inicio, fim;
double tempo, media = 0;

namespace ndn
{
  class Consumidor : noncopyable
  {
  public:
    void
    pedir(int numeroInteresse)
    {
      Interest interesse;
      interesse.setInterestLifetime(10_s);
      interesse.setMustBeFresh(true);

      switch (numeroInteresse) {
        case 0:
          interesse.setName(CONTEUDO_0);
          break;
        case 1:
          interesse.setName(CONTEUDO_1);
          break;
        case 2:
          interesse.setName(CONTEUDO_2);
          break;
        default:
          interesse.setName(CONTEUDO_3);
          break;
      }

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
    seDado(const Interest& interesse, const Data& dado)
    {
      std::cout << dado << std::endl;
    }

    void
    seNack(const Interest& interesse, const lp::Nack& nack)
    {
      std::cout << "Recebido um Nack para o interesse " << interesse
                << " pelo motivo: " << nack.getReason() << std::endl;
    }

    void
    seTimeout(const Interest& interesse)
    {
      std::cout << "Timeout para o interesse " << interesse << std::endl;
    }

  }; // class Consumidor
} // namespace ndn

int
main(int argc, char **argv)
{
  ndn::Consumidor consumidor;

  resultados.open("resultados.txt");
  resultados << "Numero de repeticoes por teste: " << NUMERO_REPETICOES << "\n\n";

  while (teste < NUMERO_TESTES) {
    clock_gettime(CLOCK_MONOTONIC, &inicio);

    for (i = 0; i <= teste; i++) {
      try {
        consumidor.pedir(i);
      }
      catch (const std::exception& e) {
        std::cerr << "ERRO: " << e.what() << std::endl;
        break;
      }
    }

    // Calculando o atraso
    clock_gettime(CLOCK_MONOTONIC, &fim);
    tempo = (fim.tv_sec - inicio.tv_sec) * 1000;
    tempo += (fim.tv_nsec - inicio.tv_nsec) / 1000000.0;

    // Salvando os resultados
    std::cout << "Atraso: " << tempo << "ms" << std::endl;
    resultados << tempo << std::endl;
    vect_resultados[contador] = tempo;

    if (contador == 24) {
      for (i = 0; i < NUMERO_REPETICOES; i++) {
        media += vect_resultados[i];
      }

      // Calculando a media
      media = media / NUMERO_REPETICOES;
      resultados << "Atraso medio (em ms): " << media << "\n\n";

      teste++;
      contador = 0;
      media = 0;
    } else contador++;

    sleep(15);
  }

  std::cout << "Teste terminado. Fechando o arquivo de resultados..." << std::endl;
  resultados.close();

  return 0;
}
