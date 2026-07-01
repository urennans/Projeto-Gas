Projeto de Monitoramento do Peso do botijão de Gás para Estimar Quantidade Restante

Sistema embarcado de monitoramento de nível de gás GLP para botijões P13, desenvolvido como projeto da disciplina de Microcontroladores (UFC) e submetido ao SBESC.

O sistema pesa continuamente o botijão via células de carga, calcula o gás restante em tempo real e envia os dados via Bluetooth para um aplicativo mobile, alertando o usuário antes do esgotamento.

Como funciona

O botijão P13 (tara 13,5 kg, marca Mangels) fica apoiado sobre uma plataforma com 4 células de carga de 50 kg em ponte de Wheatstone completa. O peso bruto é lido por um HX711 (ADC 24 bits) e processado por um STM32F103C8T6, que:


Zera a tara no boot (peso da estrutura + botijão vazio de referência)
Calcula o peso de gás restante: gas_kg = peso_bruto - 13.5
Converte para percentual: nivel_pct = (gas_kg / 13) * 100
Transmite os dados via Bluetooth (HC-05) para o app mobile em JSON
Aceita comando de tara remota disparado pelo app

Arquitetura de hardware

ComponenteFunçãoSTM32F103C8T6 (Blue Pill, 72 MHz)Microcontrolador principal4× célula de carga 50 kg (meia-ponte)Sensor de peso — ponte de Wheatstone completaHX711ADC 24 bits para leitura das célulasHC-05Comunicação Bluetooth com o app (USART1 — PA9/PA10)CH340 USB-serialMonitoramento de debug em tempo real (USART2 — PA2/PA3, 115200 baud)


Protocolo de comunicação

Pacote JSON enviado simultaneamente pelas duas UARTs (Bluetooth e debug):

json{"gas_kg":X.XX,"nivel_pct":XX,"status":"OK"}

Status possíveis: OK, ERR_HX711, ERR_SAT

Tara remota: o app envia "TARE\n" via USART1, tratado por interrupção de RX.


Aplicativo mobile

Desenvolvido em MIT App Inventor (PT-BR), duas telas:


Tela 1 — seleção do dispositivo Bluetooth (ListPicker)
Tela 2 — dashboard com nível visual do botijão (Canvas + sprites dinâmicos por faixa de nível: verde/âmbar/vermelho) e leitura de gás/percentual


O JSON é decodificado com JsonTextDecodeWithDictionaries. A leitura Bluetooth usa modo delimitador (\n, byte 10) para evitar truncamento por fragmentação de pacote (Erro 1105).


Firmware

Leitura HX711: hx711_read_average(10), 25º pulso de SCK para configuração de canal (Channel A, Gain 128)
Conversão de 24 bits com extensão de sinal (int32_t) para complemento de dois
Timeout de 200 ms (HAL_GetTick()) na leitura do HX711
Calibração: SCALE = 29400.0f, offset via tara em runtime
Saída dual UART (JSON idêntico em USART1 e USART2)

Status do projeto

-Protótipo funcional demonstrado em banca
-Firmware e app com arquitetura estável
-Calibração mecânica pendente — tensão lateral da mangueira ainda interfere na leitura de peso
-Integração com subsistema de detecção de vazamento em andamento
-Estrutura final do gabinete (marcenaria) em desenvolvimento

Ambiente de desenvolvimento

STM32CubeIDE (Linux)
MIT App Inventor

Equipe

Renan Campos · Mônica Rodrigues

Projeto acadêmico — Engenharia de Computação, UFC.
