import Ajv, { JSONSchemaType } from "ajv";

export interface IMUAxisConfig {
  enabled: boolean;
  channel: number;
  cc: number;
  defaultValue: number;
  toSerial: boolean;
  toUSBDevice: boolean;
  toUSBHost: boolean;
  sensitivity: number;
  range: number;
}

export interface IMUConfig {
  enabled: boolean;
  roll: IMUAxisConfig;
  pitch: IMUAxisConfig;
  yaw: IMUAxisConfig;
}

export interface Rp2040Config {
  command: "SAVEALL";
  filters: boolean[][];
  channels: boolean[];
  imu?: IMUConfig;
}

const ajv = new Ajv();

const imuAxisSchema: JSONSchemaType<IMUAxisConfig> = {
  type: "object",
  properties: {
    enabled: { type: "boolean" },
    channel: { type: "number", minimum: 1, maximum: 16 },
    cc: { type: "number", minimum: 0, maximum: 127 },
    defaultValue: { type: "number", minimum: 0, maximum: 127 },
    toSerial: { type: "boolean" },
    toUSBDevice: { type: "boolean" },
    toUSBHost: { type: "boolean" },
    sensitivity: { type: "number", minimum: 0.1, maximum: 10.0 },
    range: { type: "number", minimum: 5, maximum: 180 }
  },
  required: ["enabled", "channel", "cc", "defaultValue", "toSerial", "toUSBDevice", "toUSBHost", "sensitivity", "range"],
  additionalProperties: false
};

const imuSchema: JSONSchemaType<IMUConfig> = {
  type: "object",
  properties: {
    enabled: { type: "boolean" },
    roll: imuAxisSchema,
    pitch: imuAxisSchema,
    yaw: imuAxisSchema
  },
  required: ["enabled", "roll", "pitch", "yaw"],
  additionalProperties: false
};

const schema: JSONSchemaType<Rp2040Config> = {
  type: "object",
  properties: {
    command: { type: "string", const: "SAVEALL" },
    filters: {
      type: "array",
      minItems: 3,
      maxItems: 3,
      items: {
        type: "array",
        minItems: 8,
        maxItems: 8,
        items: { type: "boolean" }
      }
    },
    channels: {
      type: "array",
      minItems: 16,
      maxItems: 16,
      items: { type: "boolean" }
    },
    imu: { ...imuSchema, nullable: true }
  },
  required: ["command", "filters", "channels"],
  additionalProperties: false
};

export const validateConfig = ajv.compile(schema);
