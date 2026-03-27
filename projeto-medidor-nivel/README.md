# Projeto Medidor de Nível de Gás

## Descrição
Este subprojeto tem como objetivo estimar o nível de gás restante em um botijão de gás de cozinha a partir da medição de peso.

## Objetivo
Criar um sistema capaz de:
- medir o peso do botijão
- estimar a quantidade de gás restante
- informar ao usuário quando o nível estiver baixo

## Componentes principais
- STM32 Blue Pill
- Célula de carga
- HX711
- Display (caso utilizado)

## Funcionamento
A célula de carga mede o peso aplicado pelo botijão. O HX711 amplifica e converte esse sinal para leitura digital. A STM32 processa os dados e estima o nível de gás restante.

## Estrutura
- `docs/`: documentação específica
- `hardware/`: esquemas e componentes
- `firmware/`: código embarcado
- `data/`: dados de calibração e testes

## Status Atual
- [x] Escolha dos componentes
- [ ] Leitura do HX711
- [ ] Calibração do sistema
- [ ] Integração final