/*
 * Para compilar o codigo corretamente, colocar na linha de comando as
 * seguintes bibliotecas:
 *
 * g++ -o <nome_arquivo> consumer_timer.cpp -lndn-cxx -lboost_system
 *
 */

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <time.h>
#include <ndn-cxx/face.hpp>

#define CONTEUDO_1K                    "/ufrj/gta/moreno/controle"
#define CONTEUDO_5K                    "/ufrj/gta/moreno/computacao"
#define CONTEUDO_10K                   "/ufrj/gta/moreno/eletronica"
#define CONTEUDO_20K                   "/ufrj/gta/moreno/fisica"

#define PEDIDOS_POR_CONTEUDO           25
#define NUMERO_CONTEUDOS               4

std::ofstream resultados;
int contador = 0, conteudo = 0, i;
double vect_resultados[PEDIDOS_POR_CONTEUDO];
struct timespec inicio, fim;
double tempo, media = 0;

namespace ndn
{
  class Consumidor : noncopyable
  {
  public:
    void
    pedir()
    {
      Interest interesse;
      interesse.setInterestLifetime(10_s);
      interesse.setMustBeFresh(true);

      // Decidindo que conteudo sera pedido nessa iteracao
      switch (conteudo) {
        case 0:
          interesse.setName(CONTEUDO_1K);
          break;
        case 1:
          interesse.setName(CONTEUDO_5K);
          break;
        case 2:
          interesse.setName(CONTEUDO_10K);
          break;
        case 3:
          interesse.setName(CONTEUDO_20K);
          break;
        default:
          break;
      }

      m_face.expressInterest(interesse,
                             bind(&Consumidor::seDado, this, _1, _2),
                             bind(&Consumidor::seNack, this, _1, _2),
                             bind(&Consumidor::seTimeout, this, _1));

      clock_gettime(CLOCK_MONOTONIC, &inicio);
      std::cout << "Enviando " << interesse << std::endl;

      m_face.processEvents();

      // Resetando valores e indo para o proximo conteudo
      if (contador == 24) {
        media = 0;
        contador = 0;
        conteudo++;
      } else contador++;
    }

  private:
    Face m_face;

    void
    seDado(const Interest& interesse, const Data& dado)
    {
      // Calculando o atraso
      clock_gettime(CLOCK_MONOTONIC, &fim);
      tempo = (fim.tv_sec - inicio.tv_sec) * 1000;
      tempo += (fim.tv_nsec - inicio.tv_nsec) / 1000000.0;

      // Salvando os resultados
      std::cout << dado << std::endl;
      std::cout << "Atraso: " << tempo << "ms" << std::endl;
      resultados << tempo << std::endl;
      vect_resultados[contador] = tempo;

      if (contador == 24) {
        for (i = 0; i < NUMERO_CONTEUDOS; i++) {
          media += vect_resultados[i];
        }

        // Calculando a media
        media = media / NUMERO_CONTEUDOS;
        resultados << "Atraso medio (em ms): " << media << "\n\n";
      }
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
  resultados << "Numero de pedidos por conteudo: " << PEDIDOS_POR_CONTEUDO << "\n\n";

  while (conteudo < NUMERO_CONTEUDOS) {
    try {
      consumidor.pedir();
    }
    catch (const std::exception& e) {
      std::cerr << "ERRO: " << e.what() << std::endl;
      break;
    }

    sleep(15);
  }

  std::cout << "Teste terminado. Fechando o arquivo de resultados..." << std::endl;
  resultados.close();

  return 0;
