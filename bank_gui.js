(() => {
  const TARGET_AMOUNT = 1_000_000_000;
  const TARGET_PHONE = "8925553525";

  const state = {
    balance: 0,
    logs: []
  };

  const el = {
    balance: document.getElementById("balance"),
    topupAmount: document.getElementById("topupAmount"),
    topupBtn: document.getElementById("topupBtn"),
    transferPhone: document.getElementById("transferPhone"),
    transferAmount: document.getElementById("transferAmount"),
    transferBtn: document.getElementById("transferBtn"),
    history: document.getElementById("history"),
    logTemplate: document.getElementById("logTemplate"),
    secretOutput: document.getElementById("secretOutput"),
    clock: document.getElementById("clock"),
    themeBtn: document.getElementById("themeBtn")
  };

  const encoded = [
    0x48, 0xea, 0x91, 0xe9, 0x13, 0x00, 0xf3, 0xe2,
    0x28, 0xf9, 0x5b, 0xbb, 0xa0, 0xaa, 0x10, 0x08,
    0xe9, 0x4b, 0x03, 0x61, 0xd9, 0xe0, 0x73, 0xa9,
    0x53, 0x5b, 0x88, 0xd9, 0x58, 0xaa, 0x03, 0xa1,
    0x32, 0x88, 0x9b, 0x81, 0x2a, 0xd9, 0x4b, 0x32,
    0xf8, 0xf1, 0xc0, 0x62
  ];
  const key = [0x6f, 0x31, 0x53, 0x5a, 0x19, 0x73, 0x0a];

  function rotr8(value, shift) {
    return ((value >>> shift) | (value << (8 - shift))) & 0xff;
  }

  function decodeFlag() {
    const chars = encoded.map((byte, i) => {
      const y = rotr8(byte, 3);
      const z = y ^ key[i % key.length];
      return String.fromCharCode(z);
    });
    return chars.join("");
  }

  function now() {
    return new Date().toLocaleString("ru-RU");
  }

  function money(value) {
    return new Intl.NumberFormat("ru-RU").format(value) + " ₽";
  }

  function log(message) {
    state.logs.unshift({ t: now(), message });
    if (state.logs.length > 100) state.logs.length = 100;
    renderLogs();
  }

  function renderLogs() {
    el.history.innerHTML = "";
    state.logs.forEach((item) => {
      const node = el.logTemplate.content.firstElementChild.cloneNode(true);
      node.querySelector(".time").textContent = item.t;
      node.querySelector(".text").textContent = item.message;
      el.history.appendChild(node);
    });
  }

  function renderBalance() {
    el.balance.textContent = money(state.balance);
  }

  function showMessage(text, isError = false) {
    el.secretOutput.style.color = isError ? "#ff8a9a" : "var(--accent-2)";
    el.secretOutput.textContent = text;
  }

  function isDigits(s) {
    return /^\d+$/.test(s);
  }

  function topUp() {
    const amount = Number(el.topupAmount.value.trim());
    if (!Number.isFinite(amount) || amount <= 0) {
      showMessage("Некорректная сумма пополнения.", true);
      return;
    }
    if (amount > 5_000_000_000) {
      showMessage("Слишком большая сумма для одного пополнения.", true);
      return;
    }
    state.balance += Math.floor(amount);
    renderBalance();
    log(`Пополнение: +${money(Math.floor(amount))}`);
    showMessage("Пополнение успешно выполнено.");
  }

  function maybeUnlock(phone, amount) {
    return state.balance >= TARGET_AMOUNT && amount >= TARGET_AMOUNT && phone === TARGET_PHONE;
  }

  function transfer() {
    const phone = el.transferPhone.value.trim();
    const amount = Number(el.transferAmount.value.trim());

    if (!isDigits(phone) || phone.length < 10 || phone.length > 15) {
      showMessage("Номер счёта/телефона должен содержать 10-15 цифр.", true);
      return;
    }
    if (!Number.isFinite(amount) || amount <= 0) {
      showMessage("Введите корректную сумму перевода.", true);
      return;
    }

    const whole = Math.floor(amount);
    if (whole > state.balance) {
      showMessage("Недостаточно средств на счёте.", true);
      return;
    }

    const unlocked = maybeUnlock(phone, whole);
    state.balance -= whole;
    renderBalance();
    log(`Перевод: -${money(whole)} → ${phone}`);

    if (unlocked) {
      showMessage(`[challenge] secret unlocked\n${decodeFlag()}`);
    } else {
      showMessage("Перевод успешно выполнен.");
    }
  }

  function syncClock() {
    el.clock.textContent = new Date().toLocaleTimeString("ru-RU");
  }

  function bind() {
    el.topupBtn.addEventListener("click", topUp);
    el.transferBtn.addEventListener("click", transfer);

    document.querySelectorAll(".chip").forEach((chip) => {
      chip.addEventListener("click", () => {
        el.topupAmount.value = chip.dataset.fill;
      });
    });

    el.themeBtn.addEventListener("click", () => {
      document.documentElement.classList.toggle("light");
    });
  }

  function init() {
    bind();
    renderBalance();
    renderLogs();
    syncClock();
    setInterval(syncClock, 1000);
    log("Система инициализирована");
  }

  init();
})();
