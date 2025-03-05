// Configuração do gráfico
const ctx = document.getElementById('soundChart').getContext('2d');
const maxDataPoints = 100;
const initialData = Array(maxDataPoints).fill(0);
const initialLabels = Array(maxDataPoints).fill('');

// Debug mode
const DEBUG = false;  // Desativar modo de debug padrão para reduzir logs no console
let lastReceivedData = null; // Para armazenar a última resposta completa

// Função para log
function debugLog(message) {
    if (DEBUG) {
        console.log(`[${new Date().toLocaleTimeString()}] ${message}`);
    }
}

// Configurações do gráfico com maior precisão
const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
        y: {
            beginAtZero: true,
            max: 3.3,  // Limite máximo da escala
            precision: 0.01,  // Precisão decimal
            title: {
                display: true,
                text: 'Amplitude (V)'
            },
            ticks: {
                // Número de casas decimais nos valores do eixo Y
                callback: function(value) {
                    return value.toFixed(2) + 'V';
                }
            }
        },
        x: {
            title: {
                display: true,
                text: 'Tempo'
            }
        }
    },
    plugins: {
        tooltip: {
            callbacks: {
                label: function(context) {
                    return `Amplitude: ${context.parsed.y.toFixed(4)}V`;
                }
            }
        }
    },
    animation: {
        duration: 0  // Desativar animações para melhor desempenho
    },
    elements: {
        point: {
            radius: 0,  // Não mostrar pontos no gráfico para linhas mais suaves
            hitRadius: 8  // Área para detectar hover
        },
        line: {
            tension: 0.3  // Suavizar a linha um pouco
        }
    }
};

// Criar o gráfico
const soundChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: initialLabels,
        datasets: [{
            label: 'Nível de Som',
            data: initialData,
            borderColor: 'rgb(75, 192, 192)',
            backgroundColor: 'rgba(75, 192, 192, 0.2)',
            borderWidth: 2,
            tension: 0.2,
            fill: true
        }]
    },
    options: chartOptions
});

// Função para formatar a hora atual
function formatTime() {
    const now = new Date();
    return `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}:${now.getSeconds().toString().padStart(2, '0')}`;
}

// Função para atualizar as barras de intensidade
function updateIntensityBars(intensity) {
    // Limpa todas as barras
    for (let i = 1; i <= 5; i++) {
        document.getElementById(`bar-${i}`).className = 'intensity-bar';
    }
    
    // Acende as barras de acordo com a intensidade
    for (let i = 1; i <= intensity && i <= 5; i++) {
        const bar = document.getElementById(`bar-${i}`);
        bar.classList.add(i <= 2 ? 'low' : i <= 4 ? 'medium' : 'high');
    }
}

// Função para atualizar o nível atual
function updateCurrentLevel(value) {
    const levelBar = document.getElementById('current-level');
    const levelValue = document.getElementById('level-value');
    
    // Ajusta a altura da barra como percentual do máximo (3.3V)
    const percentage = Math.min(100, (value / 3.3) * 100);
    levelBar.style.height = `${percentage}%`;
    
    // Define a cor baseada no valor com transições mais suaves
    if (value < 0.8) {
        levelBar.style.backgroundColor = '#4caf50'; // Verde
    } else if (value < 1.6) {
        levelBar.style.backgroundColor = '#8bc34a'; // Verde claro
    } else if (value < 2.2) {
        levelBar.style.backgroundColor = '#ffeb3b'; // Amarelo
    } else if (value < 2.8) {
        levelBar.style.backgroundColor = '#ffc107'; // Amarelo escuro
    } else {
        levelBar.style.backgroundColor = '#f44336'; // Vermelho
    }
    
    // Atualiza o valor exibido com 3 casas decimais para maior precisão
    levelValue.textContent = value.toFixed(3) + 'V';
}

// Contador de falhas consecutivas
let consecutiveFailures = 0;
const maxConsecutiveFailures = 5;

// Função para buscar dados e atualizar o gráfico
function fetchAndUpdateChart() {
    debugLog("Buscando dados...");
    
    fetch('/api/data')
        .then(response => {
            if (!response.ok) {
                throw new Error(`HTTP error ${response.status}`);
            }
            debugLog("Resposta recebida com sucesso");
            return response.json();
        })
        .then(data => {
            lastReceivedData = data; // Armazena a resposta completa
            
            // Log detalhado do conteúdo recebido
            debugLog(`Dados recebidos - Modo simulação: ${data.sim_mode}`);
            debugLog(`Tamanho do histórico: ${data.history ? data.history.length : 'N/A'}`);
            debugLog(`Tamanho da fila: ${data.queue_size || 'N/A'}`);
            
            if (data.latest) {
                debugLog(`Último dado: valor=${data.latest.value}, intensidade=${data.latest.intensity}`);
            } else {
                debugLog("ALERTA: Não encontrou 'latest' na resposta");
                console.warn("Dados recebidos da API sem campo 'latest':", data);
            }
            
            // Resetar contador de falhas
            consecutiveFailures = 0;
            
            // Verificar status da conexão
            if (data.status && !data.status.connected) {
                document.getElementById('connection-status').textContent = 'Desconectado: ' + data.status.last_error;
                document.getElementById('connection-status').className = 'disconnected';
            } else {
                document.getElementById('connection-status').textContent = 'Conectado';
                document.getElementById('connection-status').className = 'connected';
            }
            
            // Atualiza último horário
            document.getElementById('last-update').textContent = formatTime();
            
            // Pega o último valor para o indicador de nível
            const latest = data.latest;
            if (latest) {
                debugLog(`Atualizando indicadores com: valor=${latest.value}, intensidade=${latest.intensity}`);
                updateCurrentLevel(latest.value);
                updateIntensityBars(latest.intensity);
            } else {
                debugLog("ERRO: Dados 'latest' não encontrados na resposta");
            }
            
            // Atualiza o gráfico com os dados históricos
            const history = data.history;
            
            if (history && history.length > 0) {
                debugLog(`Atualizando gráfico com ${history.length} pontos de dados`);
                
                // Criar arrays para valores e rótulos
                const values = history.map(item => item.value);
                const times = history.map(item => {
                    const date = new Date(item.timestamp * 1000);
                    return `${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}:${date.getSeconds().toString().padStart(2, '0')}`;
                });
                
                // Mostrar alguns dados no console para depuração
                if (values.length > 0) {
                    debugLog(`Exemplo de valores: [${values.slice(0, 3).join(", ")}...]`);
                }
                
                // Atualizar o gráfico
                soundChart.data.datasets[0].data = values;
                soundChart.data.labels = times;
                soundChart.update('none'); // Usar 'none' para animação mais rápida
                debugLog(`Gráfico atualizado com ${values.length} pontos`);
                
                // Salvar os dados para depuração
                window.SoundMonitorDebug.saveResponse(data);
            } else {
                debugLog("Nenhum dado histórico encontrado ou array vazio");
                
                // Se estamos no modo simulação mas não há dados, pode haver um problema
                if (data.sim_mode) {
                    debugLog("ALERTA: Modo de simulação ativo mas nenhum dado no histórico");
                }
            }
        })
        .catch(error => {
            consecutiveFailures++;
            console.error('Erro:', error);
            debugLog(`Erro ao buscar dados: ${error.message}`);
            
            document.getElementById('connection-status').textContent = `Desconectado (${consecutiveFailures})`;
            document.getElementById('connection-status').className = 'disconnected';
            
            // Se houver muitas falhas consecutivas, aumentar o intervalo de polling
            if (consecutiveFailures >= maxConsecutiveFailures) {
                debugLog("Muitas falhas consecutivas, aumentando intervalo de polling");
                clearInterval(pollInterval);
                pollInterval = setInterval(fetchAndUpdateChart, 5000); // Polling a cada 5 segundos
            }
        });
}

// Primeira atualização
fetchAndUpdateChart();

// Atualizar o gráfico a cada segundo
let pollInterval = setInterval(fetchAndUpdateChart, 1000);

// Adicionar feedback visual de conexão
window.addEventListener('load', function() {
    debugLog("Página carregada, iniciando...");
    // Primeira atualização imediata
    fetchAndUpdateChart();
});

// Adicionar botão para depuração no console
window.debugData = function() {
    console.log("Últimos dados recebidos:", lastReceivedData);
    if (lastReceivedData && lastReceivedData.history) {
        console.log(`Total de pontos no histórico: ${lastReceivedData.history.length}`);
    }
    console.log("Estado atual do gráfico:", soundChart.data);
};

// Adiciona função para forçar atualização
window.forceUpdate = function() {
    debugLog("Forçando atualização...");
    fetchAndUpdateChart();
};
