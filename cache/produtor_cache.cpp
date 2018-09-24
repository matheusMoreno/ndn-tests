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
      // Dado que formara os conteudos.
      static const std::string inf_content(8000, '#');    // conteudo de 8k bytes

      std::cout << "<< I: " << interesse << std::endl;

      // Pegando o nome do interesse
      Name nomeDado(interesse.getName());

      // Criando o conteudo
      shared_ptr<Data> dado = make_shared<Data>();
      dado->setName(nomeDado);
      dado->setFreshnessPeriod(1200_s); // 20 minutos
      dado->setContent(reinterpret_cast<const uint8_t*>(inf_content.data()), inf_content.size());

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
