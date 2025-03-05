/**
 * Ferramentas de depuração para o monitor de som
 */

// Adiciona na janela global
window.SoundMonitorDebug = {
    // Última resposta recebida da API
    lastResponse: null,
    
    // Histórico de respostas (até 10 últimas)
    responseHistory: [],
    
    // Salva uma resposta recebida
    saveResponse: function(response) {
        this.lastResponse = response;
        this.responseHistory.unshift(response);
        if (this.responseHistory.length > 10) {
            this.responseHistory.pop();
        }
    },
    
    // Imprime dados detalhados no console
    inspect: function() {
        console.group("Sound Monitor Debug");
        console.log("Última resposta:", this.lastResponse);
        
        if (this.lastResponse && this.lastResponse.latest) {
            console.log("Último valor:", this.lastResponse.latest.value);
            console.log("Última intensidade:", this.lastResponse.latest.intensity);
        }
        
        if (this.lastResponse && this.lastResponse.history) {
            console.log("Pontos no histórico:", this.lastResponse.history.length);
            console.log("Primeiros 5 valores:", this.lastResponse.history.slice(0, 5).map(item => item.value));
        }
        
        if (window.soundChart) {
            console.log("Dados no gráfico:", window.soundChart.data.datasets[0].data.length);
        }
        
        console.log("Status da conexão:", document.getElementById('connection-status').textContent);
        console.groupEnd();
    },
    
    // Simula dados de teste
    simulateTestData: function() {
        // Atualiza o medidor de nível
        updateCurrentLevel(Math.random() * 3.3);
        
        // Atualiza as barras de intensidade
        updateIntensityBars(Math.floor(Math.random() * 6));
        
        console.log("Dados de teste aplicados ao display");
    },
    
    // Verifica a conexão com a API
    checkConnection: function() {
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                console.log("Status da conexão:", data);
                alert("Status da conexão: " + (data.connected ? "Conectado" : "Desconectado") + 
                     "\nMensagem: " + (data.last_error || "Sem erros"));
            })
            .catch(error => {
                console.error("Erro ao verificar status:", error);
                alert("Erro ao verificar status: " + error);
            });
    }
};

// Adiciona instruções no console
console.log("%cMonitor de Som - Ferramentas de Depuração", "font-size: 14px; font-weight: bold; color: #2196F3");
console.log("Para inspecionar os últimos dados recebidos: SoundMonitorDebug.inspect()");
console.log("Para simular dados no display: SoundMonitorDebug.simulateTestData()");
console.log("Para verificar a conexão com o backend: SoundMonitorDebug.checkConnection()");
