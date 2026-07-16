/**
 * Copy to ice-config.js and fill in your TURN credentials.
 * TURN relays traffic when direct P2P fails (strict NAT / corporate firewalls).
 *
 * Free-tier options: Metered.ca, Twilio Network Traversal, self-hosted coturn.
 * Never commit ice-config.js with real credentials to git.
 */
window.AEGIS_ICE = {
    iceServers: [
        { urls: 'stun:stun.l.google.com:19302' },
        {
            urls: [
                'turn:YOUR_TURN_HOST:3478?transport=udp',
                'turn:YOUR_TURN_HOST:3478?transport=tcp'
            ],
            username: 'YOUR_TURN_USERNAME',
            credential: 'YOUR_TURN_PASSWORD'
        }
    ],
    // Use 'relay' to force TURN-only (for testing). Default 'all' tries direct first.
    iceTransportPolicy: 'all'
};
