/*
 * Para compilar o codigo corretamente, colocar na linha de comando as
 * seguintes bibliotecas:
 *
 * g++ -std=c++11 -o <nome_arquivo> consumer_timer.cpp -lndn-cxx -lboost_system
 *
 */

#include <iostream>
#include <string>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

namespace ndn
{
  class Produtor : noncopyable
  {
  public:
    void
    produzir()
    {
      m_face.setInterestFilter("/ufrj/gta/moreno",
                         bind(&Produtor::seInteresse, this, _1, _2),
                         RegisterPrefixSuccessCallback(),
                         bind(&Produtor::seFalhaRegistro, this, _1, _2));

      m_face.processEvents();
    }
  private:
    void
    seInteresse(const InterestFilter& filtro, const Interest& interesse)
    {
      // Dados que formarao os conteudos.
      static const std::string controle(1000, 'c');     // conteudo de 1k bytes
      static const std::string computacao(2000, 'c');   // conteudo de 2k bytes
      static const std::string eletronica(4000, 'e');   // conteudo de 4k bytes
      static const std::string fisica(8000, 'f');       // conteudo de 8k bytes

      std::cout << "<< I: " << interesse << std::endl;

      // Pegando o nome do interesse
      Name nomeDado(interesse.getName());
      std::string uri = nomeDado.toUri();

      // Criando o conteudo
      shared_ptr<Data> dado = make_shared<Data>();
      dado->setName(nomeDado);
      dado->setFreshnessPeriod(10_s); // 10 segundos

      std::cout << uri << std::endl;

      if (!uri.compare("/ufrj/gta/moreno/controle")) {
        dado->setContent(reinterpret_cast<const uint8_t*>(controle.data()), controle.size());
      } else if (!uri.compare("/ufrj/gta/moreno/computacao")) {
        dado->setContent(reinterpret_cast<const uint8_t*>(computacao.data()), computacao.size());
      } else if (!uri.compare("/ufrj/gta/moreno/eletronica")) {
        dado->setContent(reinterpret_cast<const uint8_t*>(eletronica.data()), eletronica.size());
      } else {
        dado->setContent(reinterpret_cast<const uint8_t*>(fisica.data()), fisica.size());
      }

      // Assinando o dado
      m_keyChain.sign(*dado);

      // Devolvendo o pacote
      std::cout << ">> D: " << *dado << std::endl;
      m_face.put(*dado);
    }

    void
    seFalhaRegistro(const Name& prefix, const std::string& razao)
    {
      std::cerr << "ERRO: Falha ao registrar o prefixo \""
                << prefix << "\" (" << razao << ")"
                << std::endl;
      m_face.shutdown();
    }

  private:
    Face m_face;
    KeyChain m_keyChain;
  }; // class Produtor
} // namespace ndn

int
main(int argc, char **argv)
{
  ndn::Produtor produtor;

  try {
    produtor.produzir();
  }
  catch (const std::exception& e) {
    std::cerr << "ERRO: " << e.what() << std::endl;
  }

  return 0;
}
