const express = require("express");
const cors = require("cors");
const { spawn } = require("child_process");
const fs = require("fs");

const app = express();
app.use(cors());
app.use(express.json());

const SIMULATOR_EXECUTABLE = "./simulator";
let simulatorProcess = null;
let latestRegisters = Array.from({ length: 32 }, (_, i) => ({ id: i, value: 0 }));
let memoryData = { data: [], stack: [], instructions: [] };
let simulatorLogs = []; // Store simulator logs
let executionComplete = false;


// **Save input code & Start Simulator**
app.post("/submit", (req, res) => {
    const code = req.body.code;
    console.log("\n[DEBUG] Received code submission:\n", code);

    fs.writeFileSync("input.mc", code);
    console.log("[DEBUG] Code saved to input.mc");

    //  Reset logs and execution state for new submission
    simulatorLogs = [];
    executionComplete = false;

    //  Terminate any existing simulator process before starting a new one
    if (simulatorProcess) {
        console.log("[DEBUG] Terminating existing simulator process...");
        simulatorProcess.kill(); 
        simulatorProcess = null;
    }

    //  Start a new simulator process
    simulatorProcess = spawn(SIMULATOR_EXECUTABLE, ["input.mc", "data.mc", "stack.mc", "instruction.mc"], { stdio: ["pipe", "pipe", "pipe"] });

    simulatorProcess.stdout.on("data", (data) => {
        let output = data.toString().trim();
        console.log("\n[DEBUG] Raw Simulator Output:\n", output);

        let outputLines = output.split("\n");

        outputLines.forEach(line => {
            //  If running "Run" (`R`), temporarily suppress register updates
            if (suppressRegisterUpdates && line.trim().startsWith("{") && line.trim().endsWith("}")) {
                latestRegisters = JSON.parse(line.trim()).registers;
                return; //  Do not print registers during "Run" mode
            }

            // Store the final register values at the last step
            if (line.trim().startsWith("{") && line.trim().endsWith("}")) {
                try {
                    const parsed = JSON.parse(line.trim());
                    if (parsed.registers && Array.isArray(parsed.registers)) {
                        latestRegisters = parsed.registers.map(reg => ({
                            id: reg.id,
                            value: reg.value
                        }));
                        if (!suppressRegisterUpdates) {
                            console.log("[DEBUG] Updated Registers:\n", latestRegisters);
                        }
                    }
                } catch (error) {
                    console.error("[ERROR] Failed to parse register JSON:", error);
                }
            } else {
                simulatorLogs.push(line);
                if (simulatorLogs.length > 50) simulatorLogs.shift(); // Keep last 50 logs
            }
        });
    });



    simulatorProcess.stderr.on("data", (data) => {
        console.error("[ERROR] Simulator Error:\n", data.toString());
        simulatorLogs.push("[ERROR] " + data.toString().trim());
    });

    simulatorProcess.on("close", () => {
        console.log("[DEBUG] Simulator process exited.");
        simulatorProcess = null;
        executionComplete = true; // Mark execution as complete
    });

    readMemoryFiles();
    res.json({ message: "Code saved & Simulator started!" });
});


// ** New API Endpoint: Fetch Simulator Logs**
app.get("/logs", (req, res) => {
    const filteredLogs = simulatorLogs.filter(log => {
        return !(log.startsWith("{") && log.endsWith("}")); // Exclude JSON logs
    });
    res.json({ logs: filteredLogs });
});

// **Read memory files**
const readMemoryFiles = () => {
    console.log("\n[DEBUG] Reading memory files...");

    const memoryFiles = ["data.mc", "stack.mc", "instruction.mc"];
    memoryData = { data: [], stack: [], instructions: [] };

    memoryFiles.forEach((file) => {
        if (fs.existsSync(file)) {
            const content = fs.readFileSync(file, "utf8").split("\n");
            const parsed = content
                .filter(line => line.trim() !== "")
                .map(line => {
                    const [address, value] = line.split(/\s+/);
                    return { address, value };
                });

            if (file === "data.mc") memoryData.data = parsed;
            else if (file === "stack.mc") memoryData.stack = parsed;
            else if (file === "instruction.mc") memoryData.instructions = parsed;

            console.log(`[DEBUG] Loaded ${file}:\n`, parsed);
        } else {
            console.warn(`[WARNING] ${file} not found!`);
        }
    });
};

// **Handle execution commands ("N", "R", "E")**
let suppressRegisterUpdates = false; // Prevent unnecessary register updates

const checkExecutionStatus = () => {
    console.log("[DEBUG] Checking execution status...");
    executionComplete = simulatorProcess === null; // Mark execution as complete when the process exits
};

app.post("/control", (req, res) => {
    const command = req.body.command;
    console.log(`\n[DEBUG] Received execution command: ${command}`);

    if (!["N", "R", "E"].includes(command)) {
        console.error("[ERROR] Invalid command received!");
        return res.status(400).json({ error: "Invalid command" });
    }

    if (!simulatorProcess) {
        console.error("[ERROR] Simulator is not running. Submit code first.");
        return res.status(400).json({ error: "Simulator is not running. Submit code first." });
    }

    if (command === "R") {
        suppressRegisterUpdates = true; // Suppress updates during "Run"
    }

    simulatorProcess.stdin.write(command + "\n");

    if (command === "N" || command === "E") {
        setTimeout(() => {
            readMemoryFiles();
            fetchRegisters();  // Update registers immediately
            checkExecutionStatus();
        }, 500);
    } else if (command === "R") {
        let checkInterval = setInterval(async () => {
            const res = await fetch("http://localhost:5000/execution-status");
            const data = await res.json();
            if (data.executionComplete) {
                clearInterval(checkInterval);
                suppressRegisterUpdates = false; // Re-enable register updates
                readMemoryFiles(); // Ensure memory updates at the end
                fetchRegisters(); // Send final registers to frontend
                checkExecutionStatus();
            }
        }, 500);
    }

    res.json({ message: `Sent command: ${command}` });
});



// **Fetch registers**
const fetchRegisters = () => {
    console.log("\n[DEBUG] Fetching latest register values...");
    
    if (!latestRegisters || latestRegisters.length === 0) {
        console.warn("[WARNING] Register values are empty! Sending default values.");
        latestRegisters = Array.from({ length: 32 }, (_, i) => ({ id: i, value: 0 }));
    }
    
    console.log("[DEBUG] Sending register values to frontend:\n", latestRegisters);
};

// Ensure `/registers` actually sends data
app.get("/registers", (req, res) => {
    fetchRegisters();  // Fetch registers before sending
    res.json({ registers: latestRegisters }); // Ensure data is sent
});

// **Fetch memory**
app.get("/memory", (req, res) => {
    console.log("\n[DEBUG] Sending memory data to frontend...");
    res.json(memoryData);
});

// **Fetch execution status**
app.get("/execution-status", (req, res) => {
    res.json({ executionComplete });
});

app.listen(5000, () => console.log("\nBackend running on port 5000"));
