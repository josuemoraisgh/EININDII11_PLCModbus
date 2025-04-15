from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import letter

# Conteúdo completo do guia – usamos uma raw string (r"""...""") para evitar problemas com escapes
pdf_content = r"""Guia Técnico: Configurando o Codesys Control Win x64 em Ambiente Multiusuário no Windows Server 2022

--------------------------------------------------
1. Visão Geral da Solução
--------------------------------------------------
Em um cenário com Windows Server 2022 (Terminal Services) e múltiplos usuários simultâneos via RDP, é possível configurar o Codesys Control Win x64 (SoftPLC) de forma que cada usuário execute seu próprio runtime isolado. Isso requer criar instâncias separadas do Codesys Control Win x64 para cada usuário, garantindo que não haja conflitos de portas, variáveis ou arquivos de configuração entre elas. Cada instância ficará em modo de licença de teste de 2 horas, independente para cada usuário (ou seja, cada um deverá reiniciar seu runtime manualmente a cada 2 horas, conforme a política de trial). A seguir, detalhamos o passo a passo de instalação, configuração de isolamento, gerenciamento de perfis/permissões, execução do runtime e melhores práticas de segurança e desempenho.

--------------------------------------------------
2. Preparação do Ambiente e Instalação
--------------------------------------------------
2.1 Configuração do Windows Server 2022:
- Configure o servidor para Serviços de Terminal (RDS), permitindo múltiplas sessões RDP simultâneas.
- Crie contas separadas para os 7 usuários (6 via RDP e 1 local).

2.2 Instalação do Codesys Development System:
- Faça o download e instale o CODESYS Development System (versão compatível com o Windows Server 2022) com privilégios de administrador.
- Certifique-se de que o atalho “CODESYS Control Win V3 x64” foi instalado corretamente.
- Realize um teste inicial executando o runtime e, em seguida, pare-o para configurar instâncias manuais.

--------------------------------------------------
3. Criando Instâncias Isoladas do Codesys Control Win x64
--------------------------------------------------
O objetivo é duplicar a instalação padrão para que cada usuário tenha uma instância separada.

3.1 Copiar os Arquivos do Runtime:
- Localize o diretório de instalação (por exemplo, "C:\Program Files\CODESYS\CODESYS Control Win V3\GatewayPLC\").
- Copie essa pasta para novos locais, criando uma estrutura como:
  C:\CODESYS\Runtimes\
      Instance_User1\
      Instance_User2\
      ...
      Instance_User7\
- Renomeie as pastas para identificar o usuário (ex.: Instance_User1 para o User1).

3.2 Definir Diretório de Trabalho Único:
- Cada instância deve apontar para um diretório de trabalho distinto.
- No arquivo de configuração "CODESYSControl.cfg" de cada instância, localize a entrada:
    [SysFile]
    Windows.WorkingDirectory=...
- Altere para apontar para um diretório exclusivo, por exemplo:
    Para User1: Windows.WorkingDirectory=C:\ProgramData\CODESYS\CODESYSControlWinV3x64\User1
    Para User2: Windows.WorkingDirectory=C:\ProgramData\CODESYS\CODESYSControlWinV3x64\User2
    ... e assim por diante.
- Crie manualmente essas pastas em "C:\ProgramData\CODESYS\CODESYSControlWinV3x64\" e copie o conteúdo do diretório de trabalho padrão para cada uma.

3.3 Isolamento de Portas de Comunicação:
- Cada instância precisa de portas exclusivas para evitar conflitos.
  * Para o Gateway (CmpGwCommDrvTcp):
    - User1: ListenPort=1217
    - User2: ListenPort=1218
    - ...
    - User7: ListenPort=1223
  * Para a porta do runtime (ex.: 11740) – se possível, configure cada instância para usar portas sequenciais.
  * Para o WebVisu (CmpWebServer):
    - User1: WebServerPortNr=8080
    - User2: WebServerPortNr=8081
    - ...
- Atribua um NodeName exclusivo para cada instância na seção [SysTarget]:
    - Exemplo: NodeName=PLC_User1 para User1, NodeName=PLC_User2 para User2, etc.

3.4 Consideração Adicional:
- Alternativamente, você pode configurar o diretório de trabalho de cada instância dentro do perfil do usuário, mas manter em ProgramData permite uma administração centralizada.

--------------------------------------------------
4. Gerenciamento de Usuários e Permissões
--------------------------------------------------
- Permissões dos Diretórios:
  * Configure as pastas de "Instance_UserX" em C:\CODESYS\Runtimes\ para que apenas o usuário correspondente e administradores tenham acesso.
  * Ajuste as pastas em "C:\ProgramData\CODESYS\CODESYSControlWinV3x64\UserX" para serem acessíveis somente pelo usuário respectivo.
- Contas de Usuário:
  * Crie contas de usuário padrão (não administradores) e adicione-os ao grupo "Remote Desktop Users".
- Isolamento entre Sessões:
  * Cada sessão RDP é isolada, garantindo que um usuário não interfira na sessão do outro.
- Perfis e Variáveis de Ambiente:
  * Cada usuário possui seu próprio perfil, evitando conflitos no uso do CODESYS IDE.
- Desative o serviço global de CODESYS Gateway se estiver ativo, para evitar conflitos com as instâncias independentes.

--------------------------------------------------
5. Iniciando o Runtime e Gerenciando a Licença de 2 Horas
--------------------------------------------------
- Cada usuário deve iniciar manualmente sua instância:
  * Execute o arquivo "CODESYSControlService.exe" na respectiva pasta (por exemplo, Instance_User1).
  * Crie atalhos no menu Iniciar ou na área de trabalho, certificando-se de que o parâmetro "Iniciar em" aponte para o diretório correto.
- Conectando o IDE ao Runtime:
  * No CODESYS IDE, configure a comunicação através de "Scan Network" ou adicione manualmente o gateway apontando para "localhost" com a porta configurada (por exemplo, 1217 para User1).
  * Verifique o NodeName no IDE para confirmar a conexão com o PLC correto.
- Gerenciamento da Licença de 2 Horas:
  * O runtime funcionará por 2 horas, após o que ele entra em expiração.
  * Cada usuário deve reiniciar sua instância manualmente para renovar o período de 2 horas.
  * Recomenda-se salvar o projeto no IDE antes de reiniciar o runtime.
  * Não há automação de reset – o usuário realiza o reinício quando necessário.

--------------------------------------------------
6. Práticas de Segurança e Desempenho
--------------------------------------------------
- Isolamento de Rede e Firewall:
  * Configure o Windows Firewall para restringir o acesso às portas dos SoftPLCs somente às sessões locais.
  * Se necessário, desative ou remova regras que permitam acesso externo às portas dos runtimes.
- Uso de Hardware Compartilhado:
  * Evite que dois runtimes acessem o mesmo dispositivo físico simultaneamente.
  * Se necessário, distribuir dispositivos físicos ou utilizar adaptadores diferentes.
- Recursos do Sistema:
  * Monitore o uso da CPU, memória, disco e tráfego de rede. Ajuste a afinidade de CPU se necessário.
- Segurança no Codesys:
  * Utilize o gerenciamento de usuários interno do Codesys, definindo senhas e níveis de acesso para cada SoftPLC.
- Manutenção e Estabilidade:
  * Mantenha o sistema operacional e o Codesys atualizados.
  * Documente o procedimento de duplicação e atualização das instâncias.
- Escalabilidade:
  * Caso a quantidade de usuários cresça, considere alternativas como virtualização ou uso de contêineres.

--------------------------------------------------
7. Organização dos Arquivos e Exemplos de Configuração
--------------------------------------------------
- Arquivos de Programa (Runtime):
  * Diretórios em "C:\CODESYS\Runtimes\Instance_UserX\" contendo o executável e arquivo de configuração.
- Diretórios de Trabalho (Dados e Configurações):
  * Estrutura em "C:\ProgramData\CODESYS\CODESYSControlWinV3x64\UserX\" com subpastas para logs, variáveis persistentes, PLC logic, e visualizações.
- Exemplo de Configuração (Arquivo CODESYSControl.cfg para User2):

  [SysFile]
  Windows.WorkingDirectory=C:\ProgramData\CODESYS\CODESYSControlWinV3x64\User2

  [CmpGwCommDrvTcp]
  ListenPort=1218

  [CmpWebServer]
  WebServerPortNr=8081

  [SysTarget]
  NodeName=PLC_User2

  (Outras seções permanecem conforme o padrão da instalação)

- Scripts de Inicialização (Opcional):
  * Criar atalhos nos perfis dos usuários para facilitar a execução dos runtimes.
- Licenciamento Futuro:
  * Atualmente utiliza a licença trial de 2 horas; para uso permanente, será necessária a aquisição de licenças apropriadas para múltiplas instâncias.

--------------------------------------------------
Conclusão
--------------------------------------------------
Seguindo os passos descritos, você terá um ambiente configurado no Windows Server 2022 com Terminal Services onde 7 usuários poderão operar instâncias isoladas do Codesys Control Win x64, cada uma com seu próprio conjunto de configurações, portas e diretório de trabalho. Assim, cada usuário pode gerenciar sua aplicação e renovar a licença de 2 horas de forma manual, garantindo a independência e o isolamento entre as instâncias.

--------------------------------------------------
Referências e Notas:
- Documentação oficial da CODESYS para instruções sobre diretórios de trabalho, configuração de portas e comportamento do runtime.
- Recomendações práticas para ambientes multiusuário usando Terminal Services.
- Orientações sobre configuração de permissões, segurança e desempenho em ambientes críticos.
"""

# Cria um canvas para o PDF com o tamanho Letter
pdf_file = "Guia_Codesys_Control_Multiusuario.pdf"
c = canvas.Canvas(pdf_file, pagesize=letter)
width, height = letter

# Configura a fonte e o tamanho
c.setFont("Helvetica", 10)

# Define margens
x_margin = 40
y_margin = 40

# Inicia a escrita do texto
text_object = c.beginText(x_margin, height - y_margin)

# Processa o conteúdo linha por linha
for line in pdf_content.splitlines():
    text_object.textLine(line)
    # Se a posição Y ficar abaixo da margem inferior, cria uma nova página
    if text_object.getY() < y_margin:
        c.drawText(text_object)
        c.showPage()
        text_object = c.beginText(x_margin, height - y_margin)
        c.setFont("Helvetica", 10)

# Finaliza e salva o PDF
c.drawText(text_object)
c.save()

print("PDF gerado com sucesso:", pdf_file)
