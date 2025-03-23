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
let dataCounter = 0;
let lastTemperature = 0;

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
    
    // Atualiza o ícone de status de conexão
    const connectionIcon = document.getElementById('connectionIcon');
    if (connectionIcon) {
        connectionIcon.className = 'status-icon connected pulse';
    }
    
    // Mostrar notificação de conexão
    showNotification('Conectado', 'Conexão com o servidor estabelecida', 'success');
});

socket.on('disconnect', function() {
    console.log('Desconectado do servidor');
    document.getElementById('status').textContent = 'Desconectado';
    document.getElementById('status').className = 'desconectado';
    
    // Para o contador de duração quando desconectado
    if (sessionInterval) clearInterval(sessionInterval);
    
    // Atualiza o ícone de status de conexão
    const connectionIcon = document.getElementById('connectionIcon');
    if (connectionIcon) {
        connectionIcon.className = 'status-icon disconnected pulse';
    }
    
    // Mostrar notificação de desconexão
    showNotification('Desconectado', 'A conexão com o servidor foi perdida', 'error');
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
const dataCounterElem = document.getElementById('dataCounter');
const tempTrendElem = document.getElementById('tempTrend');
const trendArrowElem = document.querySelector('.trend-arrow');
const alertOverlay = document.getElementById('alertOverlay');
const stateIndicator = document.getElementById('stateIndicator');
const lastAlertTimeElem = document.getElementById('lastAlertTime');
const refreshTempChartBtn = document.getElementById('refreshTempChart');
const helpButton = document.getElementById('helpButton');
const helpModal = document.getElementById('helpModal');
const closeModal = document.querySelector('.close-modal');
const themeSwitch = document.getElementById('themeSwitch');
const seguranTemperBar = document.getElementById('seguranTemperBar');

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

// ========== FUNCIONALIDADES DE TEMA ==========
// Verifica se há uma preferência de tema salva
function initTheme() {
    const savedTheme = localStorage.getItem('theme');
    if (savedTheme === 'dark') {
        document.body.classList.add('dark-theme');
        themeSwitch.checked = true;
    }
}

// Alterna entre tema claro e escuro
function toggleTheme() {
    if (document.body.classList.contains('dark-theme')) {
        document.body.classList.remove('dark-theme');
        localStorage.setItem('theme', 'light');
    } else {
        document.body.classList.add('dark-theme');
        localStorage.setItem('theme', 'dark');
    }
}

// ========== FUNÇÕES PARA NOTIFICAÇÕES ==========
// Mostra uma notificação na tela
function showNotification(title, message, type = 'info') {
    const notificationContainer = document.getElementById('notificationContainer');
    if (!notificationContainer) return;
    
    const notification = document.createElement('div');
    notification.className = `notification ${type}`;
    
    // Ícones para cada tipo de notificação
    let icon;
    switch (type) {
        case 'success': icon = '✅'; break;
        case 'warning': icon = '⚠️'; break;
        case 'error': icon = '❌'; break;
        default: icon = 'ℹ️';
    }
    
    notification.innerHTML = `
        <div class="notification-icon">${icon}</div>
        <div class="notification-content">
            <div class="notification-title">${title}</div>
            <div class="notification-message">${message}</div>
        </div>
        <div class="notification-close">&times;</div>
    `;
    
    notificationContainer.appendChild(notification);
    
    // Adicionar evento para fechar a notificação
    const closeBtn = notification.querySelector('.notification-close');
    closeBtn.addEventListener('click', () => {
        notification.style.animation = 'slideOut 0.3s forwards';
        setTimeout(() => {
            if (notification.parentNode === notificationContainer) {
                notificationContainer.removeChild(notification);
            }
        }, 300);
    });
    
    // Remove a notificação automaticamente após 5 segundos
    setTimeout(() => {
        if (notification.parentNode === notificationContainer) {
            notification.style.animation = 'slideOut 0.3s forwards';
            setTimeout(() => {
                if (notification.parentNode === notificationContainer) {
                    notificationContainer.removeChild(notification);
                }
            }, 300);
        }
    }, 5000);
}

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
    
    // Mantém apenas os últimos 60 pontos no gráfico
    if (tempChart.data.labels.length > 60) {
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
    updateStateIndicator(temperatura, limite);
}

// Atualiza o indicador de estado
function updateStateIndicator(temperatura, limite) {
    if (!stateIndicator) return;
    
    if (temperatura <= limite - 2) {
        gaugeFillElem.style.background = '#28a745'; // Verde para zonas seguras (2°C abaixo)
        stateIndicator.textContent = 'Normal';
        stateIndicator.className = '';
    } else if (temperatura >= limite) {
        gaugeFillElem.style.background = '#dc3545'; // Vermelho para acima do limite
        stateIndicator.textContent = 'ALERTA!';
        stateIndicator.className = 'danger';
    } else {
        gaugeFillElem.style.background = '#fd7e14'; // Laranja para zona de atenção
        stateIndicator.textContent = 'Atenção';
        stateIndicator.className = 'warning';
    }
    
    // Atualiza a barra de progresso para zona segura
    if (seguranTemperBar) {
        const safeTemp = limite - 2;
        const percentFromSafe = Math.max(0, Math.min(100, ((temperatura - safeTemp) / 2) * 100));
        seguranTemperBar.style.width = `${100 - percentFromSafe}%`;
    }
}

// Atualiza o indicador de tendência
function updateTrendIndicator(novaTemp) {
    if (!tempTrendElem || !trendArrowElem || lastTemperature === 0) {
        lastTemperature = novaTemp;
        return;
    }
    
    const diferenca = novaTemp - lastTemperature;
    const diferencaTexto = diferenca.toFixed(2);
    
    if (Math.abs(diferenca) < 0.05) {
        // Temperatura estável
        tempTrendElem.textContent = 'Estável';
        trendArrowElem.className = 'trend-arrow stable';
    } else if (diferenca > 0) {
        // Temperatura subindo
        tempTrendElem.textContent = `+${diferencaTexto}°C`;
        tempTrendElem.style.color = '#dc3545';
        trendArrowElem.className = 'trend-arrow up';
    } else {
        // Temperatura descendo
        tempTrendElem.textContent = `${diferencaTexto}°C`;
        tempTrendElem.style.color = '#28a745';
        trendArrowElem.className = 'trend-arrow down';
    }
    
    lastTemperature = novaTemp;
}

// Atualiza o gráfico de estatísticas
function atualizarGraficoEstatisticas(temperatura, minTemp, maxTemp, avgTemp) {
    // Se recebemos estatísticas do servidor, usamos elas
    const min = minTemp !== undefined ? minTemp : (allTemperatures.length > 0 ? Math.min(...allTemperatures) : temperatura);
    const max = maxTemp !== undefined ? maxTemp : (allTemperatures.length > 0 ? Math.max(...allTemperatures) : temperatura);
    const avg = avgTemp !== undefined ? avgTemp : (allTemperatures.length > 0 ? 
        allTemperatures.reduce((sum, val) => sum + val, 0) / allTemperatures.length : temperatura);
    
    // Adiciona à lista local apenas como fallback
    allTemperatures.push(temperatura);
    if (allTemperatures.length > 1000) {
        allTemperatures = allTemperatures.slice(-1000);
    }
    
    // Atualiza os elementos de texto
    minTempElem.textContent = min.toFixed(1) + '°C';
    maxTempElem.textContent = max.toFixed(1) + '°C';
    avgTempElem.textContent = avg.toFixed(1) + '°C';
    
    // Atualiza o gráfico
    statsChart.data.datasets[0].data = [min, avg, max];
    statsChart.update();
    
    // Aplica efeito de destaque às estatísticas
    highlightStatItem('minTempItem', temperatura === min);
    highlightStatItem('maxTempItem', temperatura === max);
}

// Aplica um efeito de destaque a um item de estatística
function highlightStatItem(itemId, shouldHighlight) {
    const item = document.getElementById(itemId);
    if (!item) return;
    
    if (shouldHighlight) {
        item.classList.add('highlight');
        setTimeout(() => {
            item.classList.remove('highlight');
        }, 1000);
    }
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

// Ativa o efeito de overlay durante alerta
function toggleAlertOverlay(isActive) {
    if (!alertOverlay) return;
    
    if (isActive) {
        alertOverlay.classList.add('active');
    } else {
        alertOverlay.classList.remove('active');
    }
}

// Ativa o ícone de alerta
function updateAlertIcon(isActive) {
    const alertIcon = document.getElementById('alertIcon');
    if (!alertIcon) return;
    
    if (isActive) {
        alertIcon.className = 'status-icon alert-active';
    } else {
        alertIcon.className = 'status-icon alert-inactive';
    }
}

// Evento para receber novos dados do servidor via WebSocket
socket.on('novo_dado', function(data) {
    const temperatura = data.temperatura;
    tempAtualElem.textContent = temperatura.toFixed(2);
    
    // Incrementa contador de dados
    dataCounter++;
    if (dataCounterElem) {
        dataCounterElem.textContent = dataCounter;
        dataCounterElem.classList.add('highlight');
        setTimeout(() => dataCounterElem.classList.remove('highlight'), 300);
    }
    
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
        const isAlerting = data.status_alerta.toLowerCase() === 'ativo';
        
        // Adicionar classe visual baseada no status
        if (isAlerting) {
            statusAlertaElem.className = 'alerta-ativo';
            // Alertar visualmente
            document.body.classList.add('alerta');
            toggleAlertOverlay(true);
            updateAlertIcon(true);
            
            // Registra o último alerta
            if (lastAlertTimeElem) {
                const now = new Date();
                lastAlertTimeElem.textContent = `Último: ${formatTime(now)}`;
            }
            
            // Mostrar notificação quando o alerta ativa
            if (data.alertas === 1 || !document.body.classList.contains('alerta')) {
                showNotification('Alerta de Temperatura', `A temperatura atingiu ${temperatura.toFixed(1)}°C, acima do limite de ${limite}°C!`, 'warning');
            }
            
            // Destaca informação sobre zona segura
            document.querySelector('.alerta-info').style.fontWeight = 'bold';
        } else {
            statusAlertaElem.className = 'alerta-inativo';
            document.body.classList.remove('alerta');
            toggleAlertOverlay(false);
            updateAlertIcon(false);
            document.querySelector('.alerta-info').style.fontWeight = 'normal';
        }
    }
    
    // Atualiza a tendência da temperatura
    updateTrendIndicator(temperatura);
    
    // Atualiza todos os gráficos
    // Atualiza todos os gráficos com as estatísticas do servidor
    atualizarGraficoTemperatura(temperatura, limite);
    atualizarMedidor(temperatura, limite);
    atualizarGraficoEstatisticas(temperatura, data.min_temp, data.max_temp, data.avg_temp);
});

// Limpa o gráfico de temperatura
function clearTempChart() {
    tempChart.data.labels = [];
    tempChart.data.datasets.forEach((dataset) => {
        dataset.data = [];
    });
    tempChart.update();
    
    showNotification('Gráfico Limpo', 'O gráfico de temperatura foi reiniciado', 'info');
}

// Inicialização
document.addEventListener('DOMContentLoaded', function() {
    console.log('Interface de monitoramento inicializada');
    
    // Inicializa a hora de início
    document.getElementById('startTime').textContent = formatTime(sessionStartTime);
    
    // Inicia o temporizador da sessão
    updateSessionDuration();
    sessionInterval = setInterval(updateSessionDuration, 1000);
    
    // Inicializa o tema
    initTheme();
    
    // Eventos para o tema
    if (themeSwitch) {
        themeSwitch.addEventListener('change', toggleTheme);
    }
    
    // inutil isso aqui
    if (refreshTempChartBtn) {
        refreshTempChartBtn.style.display = 'none'; // Oculta o botão
    }
    // Eventos para o modal de ajuda
    if (helpButton && helpModal) {
        helpButton.addEventListener('click', () => {
            helpModal.style.display = 'flex';
        });
    }
    
    if (closeModal && helpModal) {
        closeModal.addEventListener('click', () => {
            helpModal.style.display = 'none';
        });
        
        window.addEventListener('click', (e) => {
            if (e.target === helpModal) {
                helpModal.style.display = 'none';
            }
        });
    }
    
    // Adiciona classe de estilo para efeito de highlight em items
    const style = document.createElement('style');
    style.textContent = `
        .highlight {
            animation: highlight 1s ease;
        }
        @keyframes highlight {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.05); box-shadow: 0 0 20px rgba(26, 115, 232, 0.4); }
        }
    `;
    document.head.appendChild(style);

    // Solicitar estatísticas periodicamente
    setInterval(function() {
        fetch('/api/data')
            .then(response => response.json())
            .then(data => {
                if (data && data.temperatura) {
                    // Atualiza estatísticas com dados do servidor
                    minTempElem.textContent = data.min_temp ? data.min_temp.toFixed(1) + '°C' : minTempElem.textContent;
                    maxTempElem.textContent = data.max_temp ? data.max_temp.toFixed(1) + '°C' : maxTempElem.textContent;
                    avgTempElem.textContent = data.avg_temp ? data.avg_temp.toFixed(1) + '°C' : avgTempElem.textContent;
                }
            })
            .catch(err => console.error('Erro ao buscar estatísticas:', err));
    }, 30000); // A cada 30 segundos
});
