/** Default ICE config (STUN only). Add TURN credentials here for strict NAT / cross-network. See ice-config.example.js */
window.AEGIS_ICE = {
    iceServers: [{ urls: 'stun:stun.l.google.com:19302' }],
    iceTransportPolicy: 'all'
};
