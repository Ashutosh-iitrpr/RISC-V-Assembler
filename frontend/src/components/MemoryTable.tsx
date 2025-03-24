import React from "react";

interface MemoryEntry {
  address: string;
  value: string;
}

const MemoryTable: React.FC<{ memory: MemoryEntry[] }> = ({ memory }) => {
  return (
    <table className="table table-striped mt-3">
      <thead>
        <tr><th>Memory Address</th><th>Value</th></tr>
      </thead>
      <tbody>
        {memory.map((entry, index) => (
          <tr key={index}>
            <td>{entry.address}</td>
            <td>{entry.value}</td>
          </tr>
        ))}
      </tbody>
    </table>
  );
};

export default MemoryTable;
