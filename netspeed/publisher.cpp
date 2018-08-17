#include <iostream>
#include <string>

#include <ndn-cxx/face.hpp>
#include <ndn-cxx/security/key-chain.hpp>

#define PEDIDOS_POR_CONTEUDO           25
#define NUMERO_CONTEUDOS               4

int contador = 0;

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
      std::string controle(1024, 'c');        // conteudo de 1k bytes
      std::string computacao(5*1024, 'c');    // conteudo de 5k bytes
      std::string eletronica(10*1024, 'e');   // conteudo de 10k bytes
      std::string fisica(20*1024, 'f');       // conteudo de 20k bytes

      std::cout << "<< I: " << interesse << std::endl;

      // Pegando o nome do interesse
      Name nomeDado(interesse.getName());
      nomeDado.appendVersion();

      // Criando o conteudo
      shared_ptr<Data> dado = make_shared<Data>();
      dado->setName(nomeDado);
      dado->setFreshnessPeriod(10_s); // 10 segundos

      if (contador < PEDIDOS_POR_CONTEUDO * 1) {
        dado->setContent(reinterpret_cast<const uint8_t*>(controle.data()), controle.size());
      } else if (contador < PEDIDOS_POR_CONTEUDO * 2) {
        dado->setContent(reinterpret_cast<const uint8_t*>(computacao.data()), computacao.size());
      } else if (contador < PEDIDOS_POR_CONTEUDO < 3) {
        dado->setContent(reinterpret_cast<const uint8_t*>(eletronica.data()), eletronica.size());
      } else {
        dado->setContent(reinterpret_cast<const uint8_t*>(fisica.data()), fisica.size());
      }

      // Assinando o dado
      m_keyChain.sign(*dado);

      // Devolvendo o pacote
      std::cout << ">> D: " << *dado << std::endl;
      m_face.put(*dado);

      contador++;
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
