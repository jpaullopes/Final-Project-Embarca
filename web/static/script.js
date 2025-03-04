// Conecta ao servidor WebSocket
const socket = io({
    reconnection: true,
    reconnectionAttempts: Infinity,
    reconnectionDelay: 1000,
});

// Variáveis para rastreamento de tempo e sessão
const sessionStartTime = new Date();
let sessionInterval;
let allTemperatures = [];

// Feedback de conexão
socket.on('connect', function() {
    console.log('Conectado ao servidor');
    document.getElementById('status').textContent = 'Conectado';
    document.getElementById('status').className = 'conectado';
    
    // Registra o horário de início da sessão
    document.getElementById('startTime').textContent = formatTime(sessionStartTime);
    
    // Inicia o contador de duração da sessão
    if (sessionInterval) clearInterval(sessionInterval);
    sessionInterval = setInterval(updateSessionDuration, 1000);
});

socket.on('disconnect', function() {
    console.log('Desconectado do servidor');
    document.getElementById('status').textContent = 'Desconectado';
    document.getElementById('status').className = 'desconectado';
    
    // Para o contador de duração quando desconectado
    if (sessionInterval) clearInterval(sessionInterval);
});

// Seleciona os elementos HTML para atualizar os dados
const tempAtualElem = document.getElementById('tempAtual');
const alertCountElem = document.getElementById('alertCount');
const limiteTempElem = document.getElementById('limiteTemp');
const statusAlertaElem = document.getElementById('statusAlerta');
const sessionDurationElem = document.getElementById('sessionDuration');
const avgTempElem = document.getElementById('avgTemp');
const minTempElem = document.getElementById('minTemp');
const maxTempElem = document.getElementById('maxTemp');
const gaugeValueElem = document.getElementById('gaugeValue');
const gaugeFillElem = document.getElementById('gaugeFill');
const gaugeLimitElem = document.getElementById('gaugeLimit');

// Configuração de cores
const colors = {
    temp: {
        border: 'rgb(75, 192, 192)',
        background: 'rgba(75, 192, 192, 0.2)'
    },
    limit: {
        border: 'rgba(255, 99, 132, 0.8)',
        background: 'rgba(255, 99, 132, 0.1)'
    },
    stats: {
        border: 'rgb(54, 162, 235)',
        background: 'rgba(54, 162, 235, 0.2)'
    }
};

// ========== GRÁFICO PRINCIPAL DE TEMPERATURA EM TEMPO REAL ==========
const ctxTemp = document.getElementById('tempChart').getContext('2d');
const tempChart = new Chart(ctxTemp, {
    type: 'line',
    data: {
        labels: [], // Labels para o eixo x (tempo)
        datasets: [{
            label: 'Temperatura (°C)',
            data: [],
            borderColor: colors.temp.border,
            backgroundColor: colors.temp.background,
            borderWidth: 2,
            fill: true,
            tension: 0.3
        }, {
            label: 'Limite (°C)',
            data: [],
            borderColor: colors.limit.border,
            borderWidth: 1,
            borderDash: [5, 5],
            fill: false,
            pointRadius: 0
        }]
    },
    options: {
        animation: { duration: 500 },
        scales: {
            x: {
                title: { display: true, text: 'Tempo' },
                grid: { display: false }
            },
            y: {
                title: { display: true, text: 'Temperatura (°C)' },
                suggestedMin: 20,
                suggestedMax: 50
            }
        },
        plugins: {
            legend: { position: 'top' },
            tooltip: {
                backgroundColor: 'rgba(0, 0, 0, 0.8)',
                callbacks: {
                    label: function(context) {
                        return `${context.dataset.label}: ${context.raw.toFixed(2)}°C`;
                    }
                }
            }
        },
        responsive: true,
        maintainAspectRatio: false
    }
});

// ========== GRÁFICO DE HISTÓRICO DE ALERTAS ==========
// Removido

// ========== GRÁFICO DE ESTATÍSTICAS ==========
const ctxStats = document.getElementById('statsChart').getContext('2d');
const statsChart = new Chart(ctxStats, {
    type: 'bar',
    data: {
        labels: ['Mínima', 'Média', 'Máxima'],
        datasets: [{
            label: 'Estatísticas de Temperatura',
            data: [0, 0, 0],
            backgroundColor: [
                'rgba(54, 162, 235, 0.6)',
                'rgba(75, 192, 192, 0.6)',
                'rgba(255, 99, 132, 0.6)'
            ],
            borderColor: [
                'rgb(54, 162, 235)',
                'rgb(75, 192, 192)',
                'rgb(255, 99, 132)'
            ],
            borderWidth: 1
        }]
    },
    options: {
        scales: {
            y: {
                beginAtZero: false,
                title: {
                    display: true,
                    text: 'Temperatura (°C)'
                }
            }
        },
        plugins: {
            legend: {
                display: false
            }
        },
        responsive: true,
        maintainAspectRatio: false
    }
});

// Atualiza o gráfico de temperatura em tempo real
function atualizarGraficoTemperatura(temperatura, limite) {
    const agora = new Date().toLocaleTimeString();
    
    tempChart.data.labels.push(agora);
    tempChart.data.datasets[0].data.push(temperatura);
    
    // Adiciona o limite apenas se disponível
    if (limite !== undefined) {
        // Atualiza o elemento de limite do medidor
        gaugeLimitElem.textContent = limite.toFixed(1) + '°C';
        tempChart.data.datasets[1].data.push(limite);
    } else if (tempChart.data.datasets[1].data.length > 0) {
        // Se não tiver limite, copia o último valor
        const lastLimit = tempChart.data.datasets[1].data[tempChart.data.datasets[1].data.length - 1];
        tempChart.data.datasets[1].data.push(lastLimit);
    }
    
    // Adiciona a linha de segurança para desativação do alarme (2°C abaixo do limite)
    if (limite !== undefined) {
        // Cria ou atualiza conjunto de dados para a linha de zona segura, se não existir
        if (!tempChart.data.datasets[2]) {
            tempChart.data.datasets.push({
                label: 'Zona Segura (°C)',
                data: [],
                borderColor: 'rgba(40, 167, 69, 0.8)',
                borderWidth: 1,
                borderDash: [3, 3],
                fill: false,
                pointRadius: 0
            });
        }
        
        // Adiciona o valor da zona segura (limite - 2°C)
        const zonaSeguran = limite - 2;
        tempChart.data.datasets[2].data.push(zonaSeguran);
    } else if (tempChart.data.datasets[2] && tempChart.data.datasets[2].data.length > 0) {
        // Copia o último valor se não houver novo
        const lastZonaSeguran = tempChart.data.datasets[2].data[tempChart.data.datasets[2].data.length - 1];
        tempChart.data.datasets[2].data.push(lastZonaSeguran);
    }
    
    // Mantém apenas os últimos 20 pontos no gráfico
    if (tempChart.data.labels.length > 20) {
        tempChart.data.labels.shift();
        tempChart.data.datasets[0].data.shift();
        tempChart.data.datasets[1].data.shift();
        if (tempChart.data.datasets[2]) {
            tempChart.data.datasets[2].data.shift();
        }
    }
    
    tempChart.update();
}

// Atualiza o medidor de temperatura personalizado
function atualizarMedidor(temperatura, limite) {
    // Atualiza o valor exibido
    gaugeValueElem.textContent = temperatura.toFixed(1) + '°C';
    
    // Define o valor máximo do medidor como o maior entre 50 e o limite + 10
    const maxValue = Math.max(50, limite + 10);
    
    // Calcula a porcentagem que a temperatura representa no intervalo de 0 a maxValue
    const percentage = (temperatura / maxValue) * 100;
    
    // Limita a porcentagem a 100%
    const clampedPercentage = Math.min(percentage, 100);
    
    // Atualiza a rotação do preenchimento do medidor
    gaugeFillElem.style.transform = `rotate(${clampedPercentage * 1.8}deg)`;
    
    // Muda a cor do medidor com base na relação entre temperatura e limite
    if (temperatura <= limite - 2) {
        gaugeFillElem.style.backgroundColor = '#28a745'; // Verde para zonas seguras (2°C abaixo)
    } else if (temperatura >= limite) {
        gaugeFillElem.style.backgroundColor = '#dc3545'; // Vermelho para acima do limite
    } else {
        gaugeFillElem.style.backgroundColor = '#fd7e14'; // Laranja para zona de atenção
    }
}

// Atualiza o gráfico de estatísticas
function atualizarGraficoEstatisticas(temperatura) {
    // Adiciona à lista de temperaturas
    allTemperatures.push(temperatura);
    
    // Limita o histórico para não consumir muita memória
    if (allTemperatures.length > 1000) {
        allTemperatures = allTemperatures.slice(-1000);
    }
    
    // Calcula estatísticas
    const min = Math.min(...allTemperatures);
    const max = Math.max(...allTemperatures);
    const avg = allTemperatures.reduce((sum, val) => sum + val, 0) / allTemperatures.length;
    
    // Atualiza os elementos de texto
    minTempElem.textContent = min.toFixed(1) + '°C';
    maxTempElem.textContent = max.toFixed(1) + '°C';
    avgTempElem.textContent = avg.toFixed(1) + '°C';
    
    // Atualiza o gráfico
    statsChart.data.datasets[0].data = [min, avg, max];
    statsChart.update();
}

// Formata o tempo para exibição
function formatTime(date) {
    return date.toLocaleTimeString();
}

// Formata a duração em formato HH:MM:SS
function formatDuration(milliseconds) {
    const totalSeconds = Math.floor(milliseconds / 1000);
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;
    
    return [
        hours.toString().padStart(2, '0'),
        minutes.toString().padStart(2, '0'),
        seconds.toString().padStart(2, '0')
    ].join(':');
}

// Atualiza o contador de duração da sessão
function updateSessionDuration() {
    const duration = Date.now() - sessionStartTime;
    sessionDurationElem.textContent = formatDuration(duration);
}

// Evento para receber novos dados do servidor via WebSocket
socket.on('novo_dado', function(data) {
    const temperatura = data.temperatura;
    tempAtualElem.textContent = temperatura.toFixed(2);
    
    // Obtém o limite atual (ou usa o valor padrão 40)
    const limite = data.limite !== undefined ? data.limite : 40;
    
    // Atualizar contador de alertas da placa
    if (data.alertas !== undefined) {
        alertCountElem.textContent = data.alertas;
    }
    
    // Atualizar limite de temperatura se disponível
    if (data.limite !== undefined) {
        limiteTempElem.textContent = data.limite.toFixed(1);
    }
    
    // Verifica se está na zona segura (2°C abaixo do limite)
    const emZonaSegura = temperatura <= (limite - 2);
    
    // Atualizar status do alerta se disponível
    if (data.status_alerta !== undefined) {
        statusAlertaElem.textContent = data.status_alerta;
        
        // Adicionar classe visual baseada no status
        if (data.status_alerta.toLowerCase() === 'ativo') {
            statusAlertaElem.className = 'alerta-ativo';
            // Alertar visualmente
            document.body.className = 'alerta';
            setTimeout(function() {
                document.body.className = '';
            }, 1000);
            
            // Destaca informação sobre zona segura
            document.querySelector('.alerta-info').style.fontWeight = 'bold';
        } else {
            statusAlertaElem.className = 'alerta-inativo';
            document.querySelector('.alerta-info').style.fontWeight = 'normal';
        }
    }
    
    // Atualiza todos os gráficos
    atualizarGraficoTemperatura(temperatura, limite);
    atualizarMedidor(temperatura, limite);
    atualizarGraficoEstatisticas(temperatura);
});

// Inicialização
document.addEventListener('DOMContentLoaded', function() {
    console.log('Interface de monitoramento inicializada');
    
    // Inicializa a hora de início
    document.getElementById('startTime').textContent = formatTime(sessionStartTime);
    
    // Inicia o temporizador da sessão
    updateSessionDuration();
    sessionInterval = setInterval(updateSessionDuration, 1000);
});
