/**
 * Painel de diagnóstico para a interface web
 */

// Cria o painel de diagnóstico quando o documento estiver pronto
document.addEventListener('DOMContentLoaded', function() {
    createDebugPanel();
});

// Cria o painel de diagnóstico
function createDebugPanel() {
    // Cria o contêiner do painel
    const panel = document.createElement('div');
    panel.id = 'debug-panel';
    panel.className = 'debug-panel hidden';
    
    // Cabeçalho do painel
    panel.innerHTML = `
        <div class="debug-panel-header">
            <h3>Painel de Diagnóstico</h3>
            <button id="debug-panel-close">X</button>
        </div>
        <div class="debug-panel-content">
            <div class="debug-section">
                <h4>Status da Conexão</h4>
                <div id="debug-connection-status">Verificando...</div>
            </div>
            <div class="debug-section">
                <h4>Dados Recebidos</h4>
                <div id="debug-data-status">Nenhum dado</div>
            </div>
            <div class="debug-section">
                <h4>Buffer Raw (últimos 5 itens)</h4>
                <pre id="debug-raw-buffer">Carregando...</pre>
            </div>
            <div class="debug-section">
                <h4>Ações</h4>
                <button id="debug-fetch-diagnostic">Atualizar Diagnóstico</button>
                <button id="debug-force-simulation">Forçar Simulação</button>
                <button id="debug-clear-data">Limpar Dados</button>
            </div>
            <div class="debug-log">
                <h4>Log</h4>
                <div id="debug-log-entries"></div>
            </div>
        </div>
    `;
    
    // Adiciona o painel ao corpo do documento
    document.body.appendChild(panel);
    
    // Adiciona estilos
    const style = document.createElement('style');
    style.textContent = `
        .debug-panel {
            position: fixed;
            bottom: 20px;
            right: 20px;
            width: 500px;
            max-height: 80vh;
            background-color: #f5f5f5;
            border: 2px solid #2196F3;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0,0,0,0.2);
            z-index: 1000;
            overflow: hidden;
            display: flex;
            flex-direction: column;
        }
        .debug-panel.hidden {
            display: none;
        }
        .debug-panel-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px;
            background-color: #2196F3;
            color: white;
        }
        .debug-panel-header h3 {
            margin: 0;
        }
        .debug-panel-header button {
            background: none;
            border: none;
            color: white;
            font-size: 18px;
            cursor: pointer;
        }
        .debug-panel-content {
            padding: 10px;
            overflow-y: auto;
        }
        .debug-section {
            margin-bottom: 15px;
            padding: 10px;
            background-color: white;
            border-radius: 4px;
        }
        .debug-section h4 {
            margin-top: 0;
            margin-bottom: 10px;
            color: #2196F3;
        }
        .debug-log {
            height: 200px;
            overflow-y: auto;
            background-color: #272822;
            color: #f8f8f2;
            padding: 5px;
            font-family: monospace;
            font-size: 12px;
            border-radius: 4px;
        }
        #debug-log-entries {
            white-space: pre-wrap;
        }
        #debug-raw-buffer {
            max-height: 150px;
            overflow-y: auto;
            background-color: #f8f8f8;
            border: 1px solid #ddd;
            padding: 5px;
            font-family: monospace;
            font-size: 12px;
        }
        .debug-section button {
            padding: 5px 10px;
            margin-right: 5px;
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        .debug-section button:hover {
            background-color: #0b7dda;
        }
        .debug-section button:active {
            background-color: #0a6cb9;
        }
        .debug-panel-toggle {
            position: fixed;
            bottom: 20px;
            right: 20px;
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 50%;
            width: 50px;
            height: 50px;
            font-size: 24px;
            cursor: pointer;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
            z-index: 999;
        }
        .log-entry {
            margin: 2px 0;
        }
        .log-entry.error {
            color: #f44336;
        }
        .log-entry.warning {
            color: #ff9800;
        }
        .log-entry.success {
            color: #4caf50;
        }
    `;
    document.head.appendChild(style);
    
    // Adiciona botão para abrir o painel
    const toggleButton = document.createElement('button');
    toggleButton.className = 'debug-panel-toggle';
    toggleButton.textContent = 'D';
    toggleButton.title = 'Abrir painel de diagnóstico';
    document.body.appendChild(toggleButton);
    
    // Adiciona eventos
    toggleButton.addEventListener('click', function() {
        panel.classList.toggle('hidden');
        if (!panel.classList.contains('hidden')) {
            fetchDiagnosticData();
        }
    });
    
    document.getElementById('debug-panel-close').addEventListener('click', function() {
        panel.classList.add('hidden');
    });
    
    document.getElementById('debug-fetch-diagnostic').addEventListener('click', fetchDiagnosticData);
    
    document.getElementById('debug-force-simulation').addEventListener('click', function() {
        fetch('/api/toggle_simulation')
            .then(response => response.json())
            .then(data => {
                logToPanel(`Simulação: ${data.simulation_mode ? 'Ativada' : 'Desativada'}`, 'success');
                fetchDiagnosticData();
            })
            .catch(error => {
                logToPanel(`Erro ao alternar simulação: ${error}`, 'error');
            });
    });
    
    document.getElementById('debug-clear-data').addEventListener('click', function() {
        if (confirm('Limpar todos os dados? Isso reiniciará o gráfico.')) {
            soundChart.data.datasets[0].data = Array(maxDataPoints).fill(0);
            soundChart.data.labels = Array(maxDataPoints).fill('');
            soundChart.update();
            logToPanel('Dados do gráfico limpos', 'success');
        }
    });
    
    // Log inicial
    logToPanel('Painel de diagnóstico inicializado');
}

// Função para buscar dados de diagnóstico
function fetchDiagnosticData() {
    logToPanel('Buscando dados de diagnóstico...');
    
    fetch('/api/diagnostic')
        .then(response => response.json())
        .then(data => {
            // Atualiza status de conexão
            const connStatus = document.getElementById('debug-connection-status');
            if (data.connection && data.connection.connected) {
                connStatus.textContent = 'Conectado';
                connStatus.style.color = '#4caf50';
            } else {
                connStatus.textContent = 'Desconectado: ' + (data.connection ? data.connection.last_error : 'Desconhecido');
                connStatus.style.color = '#f44336';
            }
            
            // Atualiza status dos dados
            const dataStatus = document.getElementById('debug-data-status');
            if (data.latest_data && data.latest_data.timestamp) {
                const time = new Date(data.latest_data.timestamp * 1000).toLocaleTimeString();
                dataStatus.textContent = `Último dado: ${time}, Valor: ${data.latest_data.value.toFixed(4)}, Intensidade: ${data.latest_data.intensity}`;
                dataStatus.style.color = '#4caf50';
            } else {
                dataStatus.textContent = 'Nenhum dado recebido';
                dataStatus.style.color = '#f44336';
            }
            
            // Atualiza buffer raw
            const rawBuffer = document.getElementById('debug-raw-buffer');
            if (data.raw_buffer && data.raw_buffer.length > 0) {
                const last5 = data.raw_buffer.slice(-5);
                let rawText = '';
                last5.forEach(item => {
                    const time = new Date(item.time * 1000).toLocaleTimeString();
                    rawText += `[${time}] ${item.data_str.replace(/\r\n/g, '\\r\\n')}\n`;
                });
                rawBuffer.textContent = rawText;
            } else {
                rawBuffer.textContent = 'Nenhum dado raw disponível';
            }
            
            // Log de sucesso
            logToPanel(`Diagnóstico atualizado. Modo: ${data.simulation_mode ? 'Simulação' : 'Normal'}, Itens na fila: ${data.queue_size}`, 'success');
        })
        .catch(error => {
            logToPanel(`Erro ao buscar diagnóstico: ${error}`, 'error');
        });
}

// Função para adicionar entradas ao log
function logToPanel(message, type = 'info') {
    const logEntries = document.getElementById('debug-log-entries');
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    const time = new Date().toLocaleTimeString();
    entry.textContent = `[${time}] ${message}`;
    logEntries.appendChild(entry);
    logEntries.scrollTop = logEntries.scrollHeight;
    
    // Limita o número de entradas
    while (logEntries.children.length > 100) {
        logEntries.removeChild(logEntries.firstChild);
    }
}

// Adiciona ao objeto global
window.DebugPanel = {
    log: logToPanel,
    fetchDiagnostic: fetchDiagnosticData,
    showPanel: function() {
        document.getElementById('debug-panel').classList.remove('hidden');
        fetchDiagnosticData();
    },
    hidePanel: function() {
        document.getElementById('debug-panel').classList.add('hidden');
    }
};
